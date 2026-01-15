
module Hero_Game (    // Version 2.3 
    input  wire clk,           // 100 MHz del FPGA
    input  wire [3:0] row_in,
    output wire [3:0] col_out,
    output wire [6:0] display_out,
    output wire [3:0] enable_out,
    output reg  led_lost,      // led parpadea cuando se pierde en el juego
    output wire led_btn,
    output wire led_state
);

    // ---------------------------
    //  Clocks / divisores
    // ---------------------------
    wire clk0_2, clk0_5, clk1, clk2, clk5, clk10, clk15, clk20, clk20M;
    Clock_divider_multi divs (
        .clock_in(clk),
        .clk_0_2Hz(), .clk_0_5Hz(), .clk_1Hz(clk1), .clk_2Hz(clk2), .clk_5Hz(clk5),
        .clk_10Hz(), .clk_15Hz(), .clk_20Hz(), .clk_20MHz(clk20M)
    );


    // ---------------------------
    //  Keypad  — instanciación mínima
    // ---------------------------
    wire [3:0] key_code;
    wire key_valid;
    wire key_jump, key_bend, key_reset, key_power;

    keypad_4x4_controller u_keypad (
        .clk(clk20M),
        .rst_n(),        // dejar vacio
        .row(row_in),
        .col(col_out),
        .key_code(key_code),
        .key_valid(key_valid),
        .key_jump(key_jump),
        .key_bend(key_bend),
        .key_power(key_power),
        .key_reset(key_reset)
    );

    // invertimos para que los inputs al debouncer sean ACTIVE LOW
    wire btn_jump = ~key_jump;
    wire btn_bend  = ~key_bend;
    wire btn_reset = ~key_reset;
    wire btn_power = ~key_power;

    // ---------------------------
    //  Debounce de botones -> PB_state (activo HIGH)
    // ---------------------------
    wire pb_jump_state;
    wire pb_bend_state;
    wire pb_reset_state;
    wire pb_power_state;

    PushButton_Debouncer db_jump (
        .clk(clk20M), .PB(btn_jump),
        .PB_state(pb_jump_state),
        .PB_down(), .PB_up()
    );

    PushButton_Debouncer db_bend (
        .clk(clk20M), .PB(btn_bend),
        .PB_state(pb_bend_state),
        .PB_down(), .PB_up()
    );

    PushButton_Debouncer db_reset (
        .clk(clk20M), .PB(btn_reset),
        .PB_state(pb_reset_state),
        .PB_down(), .PB_up()
    );

    PushButton_Debouncer db_power (
        .clk(clk20M), .PB(btn_power),
        .PB_state(pb_power_state),
        .PB_down(), .PB_up()
    );

    assign led_btn = pb_power_state | pb_reset_state | pb_bend_state | pb_jump_state;

    
    // ---------------------------
    //  ROM_7seg instanciada para traducir direcciones 4-bit -> segmentos (anodo comun)
    //  NOTA: tu ROM debe contener las direcciones con el mapping que diste.
    // ---------------------------
    wire [6:0] rom_seg_left, rom_seg_mr, rom_seg_ml, rom_seg_right; // we will select per display
    // We'll drive ROM for the hero and for currently visible obstacle codes.
    // For simplicity use a small internal mux to choose address per display.

    // We'll instantiate ROM once and call it with required addrs when needed (combinational).
    // But easiest is to create wires holding addresses for 4 displays and feed ROM instances:
    reg [3:0] addr_left, addr_ml, addr_mr, addr_right;

    ROM_7seg rom_left  (.addr(addr_left),  .data(rom_seg_left));
    ROM_7seg rom_ml    (.addr(addr_ml),    .data(rom_seg_mr));  // middle-left (closest to hero)
    ROM_7seg rom_mr    (.addr(addr_mr),    .data(rom_seg_ml));  // middle-right
    ROM_7seg rom_right_inst (.addr(addr_right), .data(rom_seg_right)); // rightmost


    // ---------------------------
    //  Game state
    // ---------------------------
    // obstacle encoding per position: 2'b00 = EMPTY, 2'b01 = UP, 2'b10 = DOWN
    localparam EMPTY = 2'b00, UP = 2'b01, DOWN = 2'b10;

    reg [1:0] pos2; // rightmost
    reg [1:0] pos1; // middle-right
    reg [1:0] pos0; // middle-left (nearest to hero)
    // hero state: 0 = standing, 1 = jump, 2 = bend
    reg [1:0] hero_state;

    // lost flag
    reg lost;

    // power toggle retentive (use pb_power_state; pb_power_state is synchronized to clk20M)
    reg system_on = 1'b1;
    reg pb_power_prev = 1'b0;
    always @(posedge clk20M or posedge pb_reset_state) begin
        if (pb_reset_state) begin
            system_on <= 1'b1; // default ON after reset
            pb_power_prev <= 1'b0;
        end else begin
            pb_power_prev <= pb_power_state;
            if (~pb_power_prev && pb_power_state) system_on <= ~system_on; // toggle on rising edge
        end
    end

    assign top_reset = pb_reset_state | (~system_on);
    assign led_state = ~system_on;

    // Añadir led state y top reset

    // ---------------------------
    //  Random generator for obstacles
    //  Use your RandomMix module with clk5 (5 Hz) to provide random_out[3:0]
    //  We'll sample its output every shift tick (clk5) and decide new obstacle.
    // ---------------------------
    wire [3:0] rand4;
    RandomMix u_random (
        .clk(clk2),
        .rst(top_reset), // reset random on pb_reset
        .random_out(rand4)
    );

    // gap_counter en espacios vacios (mínimo 3)
    reg [2:0] gap_count; // count empty spaces since last obstacle (0..7 enough)

    // Synchronize pb_jump and pb_bend to clk5 domain and detect edge to trigger one-cycle hero action
    reg jump_sync0, jump_sync1, jump_prev;
    reg bend_sync0, bend_sync1, bend_prev;
    wire jump_edge, bend_edge;

    always @(posedge clk2 or posedge top_reset) begin
        if (top_reset) begin
            jump_sync0 <= 1'b0; jump_sync1 <= 1'b0; jump_prev <= 1'b0;
            bend_sync0 <= 1'b0; bend_sync1 <= 1'b0; bend_prev <= 1'b0;
        end else begin
            // pb_*_state are synchronized to clk20M already; sample them in clk5 domain
            jump_sync0 <= pb_jump_state;
            jump_sync1 <= jump_sync0;
            jump_prev  <= jump_sync1;

            bend_sync0 <= pb_bend_state;
            bend_sync1 <= bend_sync0;
            bend_prev  <= bend_sync1;
        end
    end

    assign jump_edge = jump_sync0; // ojo
    assign bend_edge = bend_sync0;

    // ---------------------------
    //  Main movement & collision logic (runs on clk5)
    //  At each rising edge of clk5 (when system_on and not lost) we:
    //    - compute hero_state for this tick (if jump_edge then jump, else if bend_edge then bend, else standing)
    //    - shift obstacles left: pos0 <= pos1; pos1 <= pos2; pos2 <= new_obstacle
    //    - check collision at pos0 vs hero_state AFTER shift (hero_state applies during that same tick)
    //    - enforce minimum gap_count >= 3 between obstacles
    // ---------------------------
    reg [1:0] next_new_obst;
    always @(posedge clk2 or posedge top_reset) begin
        if (top_reset) begin
            // reset game
            pos0 <= EMPTY; pos1 <= EMPTY; pos2 <= EMPTY;
            hero_state <= 2'd0;
            gap_count <= 3'd3; // allow obstacle soon
            lost <= 1'b0;
        end else begin
            // if system is off or already lost ---> do not advance obstacles; keep states.
            if (!system_on) begin
                // freeze everything, led_lost off while powered down
                // do nothing on movement ticks
            end else if (lost) begin
                // while lost, freeze movement; blink handled by clk1 below
            end else begin
                // determine hero_state for THIS tick
                if (jump_edge) hero_state <= 2'd1;      // jump for this tick
                else if (bend_edge) hero_state <= 2'd2; // bend for this tick
                else hero_state <= 2'd0;                // standing
                
                
                // decide new obstacle to insert at rightmost (pos2)
                if (gap_count < 3) begin
                    next_new_obst <= EMPTY;
                    gap_count <= gap_count + 1'b1;
                end else begin
                    // use random value to pick obstacle or empty
                    // map rand4[1:0] to candidate: 2'b00 -> UP, 2'b01 -> DOWN, others -> EMPTY
                    case (rand4[1:0])
                        2'b00: begin next_new_obst <= UP; gap_count <= 3'd0; end
                        2'b01: begin next_new_obst <= DOWN; gap_count <= 3'd0; end
                        default: begin next_new_obst <= EMPTY; gap_count <= gap_count + 1'b1; end
                    endcase
                end 


                // shift obstacles: leftward movement
                pos0 <= pos1;
                pos1 <= pos2;
                pos2 <= next_new_obst;

                // After shifting, check collision at pos0 (which is the obstacle that just arrived at hero's column)
                // collision occurs if:
                //   pos0 == DOWN  and hero_state != JUMP  -> collision
                //   pos0 == UP    and hero_state != BEND  -> collision
                // if pos0 == EMPTY -> no collision
                if ((pos0 == DOWN && hero_state != 2'd1) ||
                    (pos0 == UP   && hero_state != 2'd2)) begin
                    // Only trigger lost when there actually is an obstacle (not empty)
                    if (pos0 != EMPTY) begin
                        lost <= 1'b1;
                    end
                end

                // hero_state lasts only 1 tick; next loop it'll return to standing unless new edge arrives
            end
        end
    end


    // ---------------------------
    //  Blink led_lost when lost (use clk1)
    // ---------------------------
    reg blink;
    always @(posedge clk1 or posedge top_reset) 
    begin
        if (top_reset) begin
            blink <= 1'b0;
            led_lost <= 1'b0;
        end else begin
            if (lost) begin
                blink <= ~blink;
                led_lost <= blink;
            end else begin
                blink <= 1'b0;
                led_lost <= 1'b0;
            end
        end
    end

    // ---------------------------
    //  When system is powered off: displays OFF and power LED ON
    //  We'll drive output addresses accordingly.
    // ---------------------------
    // map internal obstacle/hero states to ROM addresses per display
    // addresses: hero=5, hero_jump=6, hero_bend=7, obst_up=0, obst_down=1, off=15

    always @(*) begin
        // default addresses = off
        addr_left  = 4'd15; // hero display (leftmost)
        addr_ml    = 4'd15; // middle-left (nearest)
        addr_mr    = 4'd15; // middle-right
        addr_right = 4'd15; // rightmost
        if (system_on) begin
            // hero display (leftmost): show according to hero_state (but if lost, keep last hero pose? We show standing)
            if (hero_state == 2'd1) addr_left = 4'd6; // hero jump
            else if (hero_state == 2'd2) addr_left = 4'd7; // hero bend
            else addr_left = 4'd5; // hero standing

            // obstacles: pos0 nearest (ml), pos1 (mr), pos2 (rightmost)
            case (pos0)
                UP:   addr_ml = 4'd0;
                DOWN: addr_ml = 4'd1;
                default: addr_ml = 4'd15;
            endcase
            case (pos1)
                UP:   addr_mr = 4'd0;
                DOWN: addr_mr = 4'd1;
                default: addr_mr = 4'd15;
            endcase
            case (pos2)
                UP:   addr_right = 4'd0;
                DOWN: addr_right = 4'd1;
                default: addr_right = 4'd15;
            endcase
        end else begin
            // system off => all OFF (addresses already 15)
            addr_left = 4'd15; addr_ml = 4'd15; addr_mr = 4'd15; addr_right = 4'd15;
        end
    end
    

    // connect ROM outputs to display scan module in the correct order:
    // note: your display module 'display' in the old code used dis1/right etc.
    // here we assume display expects (dis1,dis2,dis3,dis4) in the same order you used previously.
    // We will wire as: dis1 = rightmost, dis2 = middle-right, dis3 = middle-left, dis4 = leftmost(hero)
    display salida_display (
        .clk(clk20M),
        .dis1(rom_seg_ml),   // rightmost
        .dis2(rom_seg_mr),      // middle-right  (note rom instance order mapping above)
        .dis3(rom_seg_left),      // middle-left
        .dis4(rom_seg_right),    // leftmost (hero)
        .out(display_out),
        .en(enable_out)
    );
    
    // ---------------------------
    //  Initial values and reset on configuration reload
    // ---------------------------
    initial begin
        pos0 = EMPTY; pos1 = EMPTY; pos2 = EMPTY;
        hero_state = 2'd0;
        gap_count = 3'd3;
        lost = 1'b0;
        system_on = 1'b1;
        led_lost = 1'b0;
    end 

endmodule




// ROM combinacional de 16 x 7 bits

module ROM_7seg (
    input  wire [3:0] addr,
    output reg  [6:0] data
);

    always @(*) begin
        case (addr)
            
            4'd0  : data = 7'b0011100; // cuadrado arriba
            4'd1  : data = 7'b0100011; // cuadrado abajo
            4'd2  : data = 7'b0011011; // slash
            4'd3  : data = 7'b0101101; // inv slash
            4'd4  : data = 7'b1111001; // | der
            4'd5  : data = 7'b1001111; // | izq
            4'd6  : data = 7'b1111110; // -
            4'd7  : data = 7'b1110111; // _
            4'd8  : data = 7'b0010010; // 5
            4'd9  : data = 7'b0101010; // M
            4'd10 : data = 7'b0001001; // H 
            4'd11 : data = 7'b0110110; // horizontales
            4'd12 : data = 7'b0001111; // tal
            4'd13 : data = 7'b0111001; // lat
            4'd14 : data = 7'b1011011; // tu ta
            4'd15 : data = 7'b1111111; // apagado
            
            default: data = 7'b1111111; // apagado

        endcase
    end

endmodule





// Generador de números pseudoaleatorios
module RandomMix (
    input  wire clk,
    input  wire rst,
    output reg  [3:0] random_out
);
    reg [3:0] lfsrA = 4'b0000;
    reg [4:0] lfsrB = 5'b00000;

    wire fbA = lfsrA[3] ^ lfsrA[2];
    wire fbB = lfsrB[4] ^ lfsrB[2];

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            lfsrA <= 4'b1011;
            lfsrB <= 5'b11001;
            random_out <= 4'd0;
        end else begin
            lfsrA <= {lfsrA[2:0], fbA};
            lfsrB <= {lfsrB[3:0], fbB};
            random_out <= (lfsrA ^ lfsrB[3:0]) + lfsrB[2:0];
        end
    end
endmodule




// Modulo debouncer para una señal de entrada de un push bottom

module PushButton_Debouncer(
    input clk, // Ingresar CLK de 20 MHz
    input PB,  // "PB" is the glitchy, asynchronous to clk, active low push-button signal

    // from which we make three outputs, all synchronous to the clock
    output reg PB_state,  // 1 as long as the push-button is active (i.e. pushed down)
    output PB_down,  // 1 for one clock cycle when the push-button goes down (i.e. just pushed)
    output PB_up   // 1 for one clock cycle when the push-button goes up (i.e. just released)
);

// First use two flip-flops to synchronize the PB signal the "clk" clock domain
reg PB_sync_0;  always @(posedge clk) PB_sync_0 <= ~PB;  // invert PB to make PB_sync_0 active high
reg PB_sync_1;  always @(posedge clk) PB_sync_1 <= PB_sync_0;

// Next declare a 16-bit counter
reg [15:0] PB_cnt;

// When the push-button is pushed or released, we increment the counter
// The counter has to be maxed out before we decide that the push-button state has changed

wire PB_idle = (PB_state==PB_sync_1);
wire PB_cnt_max = &PB_cnt;	// true when all bits of PB_cnt are 1's

always @(posedge clk)
if(PB_idle)
    PB_cnt <= 0;  // nothing's going on
else
begin
    PB_cnt <= PB_cnt + 16'd1;  // something's going on, increment the counter
    if(PB_cnt_max) PB_state <= ~PB_state;  // if the counter is maxed out, PB changed!
end

assign PB_down = ~PB_idle & PB_cnt_max & ~PB_state;
assign PB_up   = ~PB_idle & PB_cnt_max &  PB_state;
endmodule




// ---------------------- Multi Clock Divider ------------------------
module Clock_divider_multi (
    input  wire clock_in,       // reloj principal FPGA ( 100 MHz)
    output reg clk_0_2Hz,
    output reg clk_0_5Hz,
    output reg clk_1Hz,
    output reg clk_2Hz,
    output reg clk_5Hz,
    output reg clk_10Hz,
    output reg clk_15Hz,
    output reg clk_20Hz,
    output reg clk_20MHz
);

    // Parámetros de división 
    parameter FREQ_IN   = 100_000_000; // 100 MHz
    parameter DIV_0_2Hz = FREQ_IN / 0.2; 
    parameter DIV_0_5Hz   = FREQ_IN / 0.5;
    parameter DIV_1Hz   = FREQ_IN / 11;
    parameter DIV_2Hz  = FREQ_IN / 2;
    parameter DIV_5Hz  = FREQ_IN / 5;
    parameter DIV_10Hz  = FREQ_IN / 10;
    parameter DIV_15Hz  = FREQ_IN / 15;
    parameter DIV_20Hz  = FREQ_IN / 20;
    parameter DIV_20MHz  = FREQ_IN / 20_000_000;

    // Contadores
    reg [31:0] cnt_0_2Hz = 0;
    reg [31:0] cnt_0_5Hz   = 0;
    reg [31:0] cnt_1Hz   = 0;
    reg [31:0] cnt_2Hz  = 0;
    reg [31:0] cnt_5Hz  = 0;
    reg [31:0] cnt_10Hz  = 0;
    reg [31:0] cnt_15Hz  = 0;
    reg [31:0] cnt_20Hz  = 0;
    reg [31:0] cnt_20MHz  = 0;

    always @(posedge clock_in) 
    begin
        // --- 0.2 Hz ---
        if (cnt_0_2Hz >= (DIV_0_2Hz-1)) 
            cnt_0_2Hz <= 0;
        else 
            cnt_0_2Hz <= cnt_0_2Hz + 1;
            clk_0_2Hz <= (cnt_0_2Hz < DIV_0_2Hz/2);

        // --- 0.5 Hz ---
        if (cnt_0_5Hz >= (DIV_0_5Hz-1)) 
            cnt_0_5Hz <= 0;
        else 
            cnt_0_5Hz <= cnt_0_5Hz + 1;
            clk_0_5Hz <= (cnt_0_5Hz < DIV_0_5Hz/2);

        // --- 1 Hz ---
        if (cnt_1Hz >= (DIV_1Hz-1)) 
            cnt_1Hz <= 0;
        else 
            cnt_1Hz <= cnt_1Hz + 1;
            clk_1Hz <= (cnt_1Hz < DIV_1Hz/2);

        // --- 2 Hz ---
        if (cnt_2Hz >= (DIV_2Hz-1)) 
            cnt_2Hz <= 0;
        else 
            cnt_2Hz <= cnt_2Hz + 1;
            clk_2Hz <= (cnt_2Hz < DIV_2Hz/2);

        // --- 5 Hz ---
        if (cnt_5Hz >= (DIV_5Hz-1)) 
            cnt_5Hz <= 0;
        else 
            cnt_5Hz <= cnt_5Hz + 1;
            clk_5Hz <= (cnt_5Hz < DIV_5Hz/2);

        // --- 10 Hz ---
        if (cnt_10Hz >= (DIV_10Hz-1)) 
            cnt_10Hz <= 0;
        else 
            cnt_10Hz <= cnt_10Hz + 1;
            clk_10Hz <= (cnt_10Hz < DIV_10Hz/2);

        // --- 15 Hz ---
        if (cnt_15Hz >= (DIV_15Hz-1)) 
            cnt_15Hz <= 0;
        else 
            cnt_15Hz <= cnt_15Hz + 1;
            clk_15Hz <= (cnt_15Hz < DIV_15Hz/2);

        // --- 20 Hz ---
        if (cnt_20Hz >= (DIV_20Hz-1)) 
            cnt_20Hz <= 0;
        else 
            cnt_20Hz <= cnt_20Hz + 1;
            clk_20Hz <= (cnt_20Hz < DIV_20Hz/2);

        // --- 20 MHz ---
        if (cnt_20MHz >= (DIV_20MHz-1)) 
            cnt_20MHz <= 0;
        else 
            cnt_20MHz <= cnt_20MHz + 1;
            clk_20MHz <= (cnt_20MHz < DIV_20MHz/2);
    end

endmodule




module display(
    input clk, // 20 MHz
    input [6:0] dis1,
    input [6:0] dis2,
    input [6:0] dis3,
    input [6:0] dis4,

    output reg [6:0] out, 
    output reg [3:0] en

);

    always @(posedge clk) 
    case (en)
        4'b0001:en = 4'b0010;
        4'b0010:en = 4'b0100;
        4'b0100:en = 4'b1000;
        4'b1000:en = 4'b0001; 
        default:en = 4'b0001;
    endcase

    always @(posedge clk) 
    begin
        if (en == 4'b0001) begin
            out <= dis1;
        end else if (en == 4'b0010) begin
            out <= dis2;
        end else if (en == 4'b0100) begin
            out <= dis3;
        end else if (en == 4'b1000) begin
            out <= dis4;
        end else begin
          out <= 7'd1;
        end
    end

endmodule





// Entradas del keypad

module keypad_4x4_controller (
    input  wire clk,         // reloj del sistema (100 MHz)
    input  wire rst_n,       // reset activo bajo
    input  wire [3:0] row,   // filas del keypad (entradas)
    output reg  [3:0] col,   // columnas del keypad (salidas)
    output reg  [3:0] key_code,  // código de tecla presionada
    output reg  key_valid,       // 1 si una tecla está activa
    output wire key_jump,       // tecla 'A'
    output wire key_bend,       // tecla 'B'
    output wire key_power,      // tecla 'C'
    output wire key_reset       // tecla 'D'
);

    // Mapa lógico de teclas (columna x fila)
    // [fila][columna]
    // 1  2  3  A
    // 4  5  6  B
    // 7  8  9  C
    // *  0  #  D

    reg [1:0] col_index;
    reg [19:0] scan_counter;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            scan_counter <= 0;
            col_index    <= 0;
            col          <= 4'b1110;
        end else begin
            scan_counter <= scan_counter + 1;
            if (scan_counter == 20'd50000) begin // cambio cada 50000 ciclos -> ~2 kHz
                scan_counter <= 0;
                col_index <= col_index + 1;
                case (col_index)
                    2'd0: col <= 4'b1110;
                    2'd1: col <= 4'b1101;
                    2'd2: col <= 4'b1011;
                    2'd3: col <= 4'b0111;
                endcase
            end
        end
    end

    // Decodificación de tecla presionada
    always @(*) begin
        key_valid = 1'b0;
        key_code  = 4'h0;

        case (col)
            4'b1110: begin
                case (row)
                    4'b1110: begin key_code = 4'h1; key_valid = 1; end
                    4'b1101: begin key_code = 4'h4; key_valid = 1; end
                    4'b1011: begin key_code = 4'h7; key_valid = 1; end
                    4'b0111: begin key_code = 4'hE; key_valid = 1; end
                endcase
            end
            4'b1101: begin
                case (row)
                    4'b1110: begin key_code = 4'h2; key_valid = 1; end
                    4'b1101: begin key_code = 4'h5; key_valid = 1; end
                    4'b1011: begin key_code = 4'h8; key_valid = 1; end
                    4'b0111: begin key_code = 4'h0; key_valid = 1; end
                endcase
            end
            4'b1011: begin
                case (row)
                    4'b1110: begin key_code = 4'h3; key_valid = 1; end
                    4'b1101: begin key_code = 4'h6; key_valid = 1; end
                    4'b1011: begin key_code = 4'h9; key_valid = 1; end
                    4'b0111: begin key_code = 4'hF; key_valid = 1; end
                endcase
            end
            4'b0111: begin
                case (row)
                    4'b1110: begin key_code = 4'hA; key_valid = 1; end
                    4'b1101: begin key_code = 4'hB; key_valid = 1; end
                    4'b1011: begin key_code = 4'hC; key_valid = 1; end
                    4'b0111: begin key_code = 4'hD; key_valid = 1; end
                endcase
            end
        endcase
    end

    // Señales específicas para mi sistema
    assign key_jump = (key_valid && key_code == 4'h1);
    assign key_reset = (key_valid && key_code == 4'h7);
    assign key_power = (key_valid && key_code == 4'hE);
    assign key_bend = (key_valid && key_code == 4'h4);

endmodule




