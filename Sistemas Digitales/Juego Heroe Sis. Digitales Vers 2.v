

// ROM combinacional de 16 x 7 bits
// Los patrones para los displays

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
            4'd6  : data = 7'b1111110; // - up
            4'd7  : data = 7'b1110111; // _ down
            4'd8  : data = 7'b0111111; // - middle
            4'd9  : data = 7'b1101111; // | izq small
            4'd10 : data = 7'b0111001; // -| 
            4'd11 : data = 7'b1111000; // - up |
            4'd12 : data = 7'b0001111; // |-
            4'd13 : data = 7'b0111001; // -|
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




// Multiplexado para 4 displays ANODO COMUN

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





// Entradas del keypad mejorado 3.0

module keypad_4x4_controller (
    input  wire        clk,       // Ej: 20 MHz desde top
    input  wire        rst_n,     // active low reset (conectar a 1'b1 si no se usa)
    input  wire [3:0]  row,       // filas (entradas con pull-up)
    output reg  [3:0]  col,       // columnas (salidas, active low)
    output reg  [3:0]  key_code,  // código de tecla (hex)
    output reg         key_valid  // 1 cuando hay al menos una tecla presionada
);

    // ----------------------------
    // Parámetros (ajustables)
    // ----------------------------
    localparam integer SCAN_MAX = 20000;       // ajuste para clk=20MHz
    localparam integer RELEASE_FRAMES = 3;     // frames para considerar liberada

    // ----------------------------
    // Señales internas
    // ----------------------------
    reg [15:0] scan_cnt;
    reg [1:0]  col_idx;
    reg [15:0] key_seen;        // bit por tecla: 1 = detectada en el frame actual
    reg [15:0] key_state;       // latched state: 1 = pressed
    reg [3:0]  miss_frames [0:15]; // contador de ausencia por tecla (4 bits cada uno)

    integer i;
    reg frame_tick_prev;
    reg found;


    // Inicialización en reset
    integer j;
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            scan_cnt <= 16'd0;
            col_idx  <= 2'd0;
            col      <= 4'b1110;
            key_seen <= 16'd0;
            key_state <= 16'd0;
            key_code <= 4'h0;
            key_valid <= 1'b0;
            frame_tick_prev <= 1'b0;
            for (j = 0; j < 16; j = j + 1) begin
                miss_frames[j] <= 4'd0;
            end
        end else begin
            // ----------------------------
            // SCAN COUNTER + COLUMN DRIVE
            // ----------------------------
            if (scan_cnt >= SCAN_MAX - 1) begin
                scan_cnt <= 16'd0;
                // advance column index (wrap 0..3)
                if (col_idx == 2'd3) col_idx <= 2'd0;
                else col_idx <= col_idx + 1'b1;

                // set column outputs (active low)
                case (col_idx)
                    2'd0: col <= 4'b1110;
                    2'd1: col <= 4'b1101;
                    2'd2: col <= 4'b1011;
                    2'd3: col <= 4'b0111;
                    default: col <= 4'b1110;
                endcase
            end else begin
                scan_cnt <= scan_cnt + 1'b1;
            end

            // ----------------------------
            // CAPTURA: cuando la columna actual está activa, marcar key_seen bits
            // row: se espera active-low (1 = no press, 0 = pressed)
            // Mapeo de índices k: col_idx*4 + fila_index
            // ----------------------------
            // Solo evaluar cuando la columna está estable (evitar metastabilidad por cambio de col)
            // Notar: usamos el valor actual de col_idx
            if (col_idx == 2'd0) begin
                if (row == 4'b1110) key_seen[0]  <= 1'b1;
                if (row == 4'b1101) key_seen[4]  <= 1'b1;
                if (row == 4'b1011) key_seen[8]  <= 1'b1;
                if (row == 4'b0111) key_seen[12] <= 1'b1;
            end else if (col_idx == 2'd1) begin
                if (row == 4'b1110) key_seen[1]  <= 1'b1;
                if (row == 4'b1101) key_seen[5]  <= 1'b1;
                if (row == 4'b1011) key_seen[9]  <= 1'b1;
                if (row == 4'b0111) key_seen[13] <= 1'b1;
            end else if (col_idx == 2'd2) begin
                if (row == 4'b1110) key_seen[2]  <= 1'b1;
                if (row == 4'b1101) key_seen[6]  <= 1'b1;
                if (row == 4'b1011) key_seen[10] <= 1'b1;
                if (row == 4'b0111) key_seen[14] <= 1'b1;
            end else begin // col_idx == 3
                if (row == 4'b1110) key_seen[3]  <= 1'b1;
                if (row == 4'b1101) key_seen[7]  <= 1'b1;
                if (row == 4'b1011) key_seen[11] <= 1'b1;
                if (row == 4'b0111) key_seen[15] <= 1'b1;
            end

            // ----------------------------
            // FRAME TICK: detectamos comienzo de nuevo frame cuando scan_cnt==0 y col_idx==0
            // (es decir acabamos de completar columna 3 y volvemos a 0)
            // ----------------------------
            // compute frame tick
            // es true durante el ciclo donde scan_cnt==0 and col_idx==0
            // guardamos prev para usar solo un pulso
            if ((scan_cnt == 16'd0) && (col_idx == 2'd0) && (frame_tick_prev == 1'b0)) begin
                // -------- actualizar estados latched basado en key_seen ----------
                for (i = 0; i < 16; i = i + 1) begin
                    if (key_seen[i]) begin
                        key_state[i] <= 1'b1;
                        miss_frames[i] <= 4'd0;
                    end else begin
                        // not seen this frame -> increase miss counter
                        if (miss_frames[i] < 4'hF) miss_frames[i] <= miss_frames[i] + 1'b1;
                        if (miss_frames[i] >= RELEASE_FRAMES) key_state[i] <= 1'b0;
                    end
                    // limpiar bit seen para el próximo frame
                    key_seen[i] <= 1'b0;
                end

                // -------- generar key_code/key_valid percibiendo la primera tecla activa (prioridad baja->alta índice) ----------
                key_valid <= 1'b0;
                key_code  <= 4'h0;
                // Encontrar la primera key_state activa
                // usamos un flag 'found' para no usar disable for
                

                found = 1'b0;
                for (i = 0; i < 16; i = i + 1) begin
                    if ((found == 1'b0) && (key_state[i] == 1'b1)) begin
                        key_valid <= 1'b1;
                        case (i)
                            0: key_code <= 4'h0;
                            1: key_code <= 4'h1;
                            2: key_code <= 4'h2;
                            3: key_code <= 4'h3;
                            4: key_code <= 4'h4;
                            5: key_code <= 4'h5;
                            6: key_code <= 4'h6;
                            7: key_code <= 4'h7;
                            8: key_code <= 4'h8;
                            9: key_code <= 4'h9;
                            10: key_code <= 4'hA;
                            11: key_code <= 4'hB;
                            12: key_code <= 4'hC; 
                            13: key_code <= 4'hD;
                            14: key_code <= 4'hE; // *
                            15: key_code <= 4'hF; // #
                            default: key_code <= 4'h0;
                        endcase
                        found = 1'b1;
                    end
                end
            end

            // update frame_tick_prev
            frame_tick_prev <= ((scan_cnt == 16'd0) && (col_idx == 2'd0)) ? 1'b1 : 1'b0;
        end
    end

endmodule





// Isaias Matos
// Pract VII: Juego del heroe 

// Hero_Game.v  -- Version 3 (añadido modo heroe pequeño)
// Cambios: selector de personaje (key_select), hero_small toggle, nuevos obstaculos para modo pequeño.

module Hero_Game (    // Version 3
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
    //  Keypad  — instanciación
    // ---------------------------
    wire [3:0] key_code;
    wire key_valid;

    keypad_4x4_controller u_keypad (
        .clk(clk20M),
        .rst_n(1'b1),  // Obligatorio un 1       
        .row(row_in),
        .col(col_out),
        .key_code(key_code),
        .key_valid(key_valid)
    );

    // ---------------------------
    //  MAPPING: key_code -> funciones (usar key_valid)
    //  (Puedes ajustar los hex si quieres otro mapeo)
    // ---------------------------
    wire key_jump      = (key_valid && key_code == 4'hA); // A
    wire key_bend      = (key_valid && key_code == 4'hB); // B
    wire key_reset     = (key_valid && key_code == 4'hC); // C
    wire key_power     = (key_valid && key_code == 4'hD); // D
    wire key_select    = (key_valid && key_code == 4'h3); // 3

    // invertimos para que los inputs al debouncer sean ACTIVE LOW
    wire btn_jump  = ~key_jump;
    wire btn_bend  = ~key_bend;
    wire btn_reset = ~key_reset;
    wire btn_power = ~key_power;
    wire btn_select = ~key_select;

    // ---------------------------
    //  Debounce de botones -> PB_state (activo HIGH)
    // ---------------------------
    wire pb_jump_state;
    wire pb_bend_state;
    wire pb_reset_state;
    wire pb_power_state;
    wire pb_select_state;

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

    PushButton_Debouncer db_select (
        .clk(clk20M), .PB(btn_select),
        .PB_state(pb_select_state),
        .PB_down(), .PB_up()
    );

    // Led para visualizar cuando se presiona un botón.
    assign led_btn = pb_power_state | pb_reset_state | pb_bend_state | pb_jump_state | pb_select_state;

    // ---------------------------
    //  ROM_7seg instanciada para traducir direcciones 4-bit -> segmentos (anodo comun)
    // ---------------------------
    wire [6:0] rom_seg_left, rom_seg_mr, rom_seg_ml, rom_seg_right;
    reg  [3:0] addr_left, addr_ml, addr_mr, addr_right;

    ROM_7seg rom_left  (.addr(addr_left),  .data(rom_seg_left));
    ROM_7seg rom_ml    (.addr(addr_ml),    .data(rom_seg_mr));   // middle-left
    ROM_7seg rom_mr    (.addr(addr_mr),    .data(rom_seg_ml));   // middle-right
    ROM_7seg rom_right_inst (.addr(addr_right), .data(rom_seg_right)); // rightmost

    // ---------------------------
    //  Game state
    // ---------------------------
    // Obstacle codes: we keep small numeric codes for compatibility with ROM addresses:
    // UP = 4'd0, DOWN = 4'd1, obs_3 = 4'd2, obs_4 = 4'd11, obs_5 = 4'd13, EMPTY = 4'd15 (OFF)
    localparam EMPTY4 = 4'd15;
    localparam UP4    = 4'd0;
    localparam DOWN4  = 4'd1;
    localparam OBS3   = 4'd2;
    localparam OBS4   = 4'd11;
    localparam OBS5   = 4'd13;

    // hero_state: 0 = standing, 1 = jump, 2 = bend  (same encoding)
    reg [1:0] hero_state;

    // posX now 4-bit to hold new obstacle codes
    reg [3:0] pos2; // rightmost
    reg [3:0] pos1; // middle-right
    reg [3:0] pos0; // middle-left (nearest to hero)

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

    // hero_small toggle (retentive). Detect rising edge of pb_select_state in clk20M domain
    reg hero_small = 1'b0; // 0 = big (default), 1 = small
    reg pb_select_prev = 1'b0;
    reg select_edge = 1'b0;
    always @(posedge clk20M or posedge pb_reset_state) begin
        if (pb_reset_state) begin
            pb_select_prev <= 1'b0;
            hero_small <= 1'b0;
            select_edge <= 1'b0;
        end else begin
            pb_select_prev <= pb_select_state;
            select_edge <= (~pb_select_prev & pb_select_state);
            if (~pb_select_prev & pb_select_state) begin
                hero_small <= ~hero_small; // toggle mode
            end
        end
    end

    // top_reset: includes pb_reset, select_edge (soft reset on mode change) and power off
    wire top_soft_reset = pb_reset_state | select_edge;
    wire top_reset = top_soft_reset | (~system_on);

    assign led_state = ~system_on;

    // ---------------------------
    //  Random generator for obstacles
    //  Use RandomMix with clk2 (you used clk2 earlier); reset with top_reset so changing mode restarts sequence
    // ---------------------------
    wire [3:0] rand4;
    RandomMix u_random (
        .clk(clk2),
        .rst(top_reset),
        .random_out(rand4)
    );

    // gap_counter en espacios vacios (mínimo 3)
    reg [2:0] gap_count; // count empty spaces since last obstacle (0..7 enough)

    // Synchronize pb_jump and pb_bend to clk2 domain and detect edge to trigger one-cycle hero action
    reg jump_sync0, jump_sync1, jump_prev;
    reg bend_sync0, bend_sync1, bend_prev;
    wire jump_edge, bend_edge;

    always @(posedge clk2 or posedge top_reset) begin
        if (top_reset) begin
            jump_sync0 <= 1'b0; jump_sync1 <= 1'b0; jump_prev <= 1'b0;
            bend_sync0 <= 1'b0; bend_sync1 <= 1'b0; bend_prev <= 1'b0;
        end else begin
            // pb_*_state are synchronized to clk20M already; sample them in clk2 domain
            jump_sync0 <= pb_jump_state;
            jump_sync1 <= jump_sync0;
            jump_prev  <= jump_sync1;

            bend_sync0 <= pb_bend_state;
            bend_sync1 <= bend_sync0;
            bend_prev  <= bend_sync1;
        end
    end

    // Use sampled level as immediate action for this tick (1-tick actions)
    assign jump_edge = jump_sync0;
    assign bend_edge = bend_sync0;

    // ---------------------------
    //  Main movement & collision logic (runs on clk2)
    // ---------------------------
    //  At each rising edge of clk2 (when system_on and not lost) we:
    //   - compute hero_state for this tick (if jump_edge then jump, else if bend_edge then bend, else standing)
    //   - shift obstacles left: pos0 <= pos1; pos1 <= pos2; pos2 <= new_obstacle
    //   - check collision at pos0 vs hero_state AFTER shift (hero_state applies during that same tick)
    //   - enforce minimum gap_count >= 3 between obstacles
    // ---------------------------
    reg [3:0] next_new_obst;
    always @(posedge clk2 or posedge top_reset) begin
        if (top_reset) begin
            // reset game
            pos0 <= EMPTY4; pos1 <= EMPTY4; pos2 <= EMPTY4;
            hero_state <= 2'd0;
            gap_count <= 3'd3; // allow obstacle soon
            lost <= 1'b0;
        end else begin
            // if system is off or already lost ---> do not advance obstacles; keep states.
            if (!system_on) begin
                // freeze everything
            end else if (lost) begin
                // freeze while lost; wait for reset/select to restart
            end else begin
                // determine hero_state for THIS tick
                // For small hero, we still track hero_state (0/1/2) but mapping to positions is different later
                if (jump_edge) hero_state <= 2'd1;      // jump for this tick (or 'arriba' when small)
                else if (bend_edge) hero_state <= 2'd2; // bend for this tick (or 'medio' when small)
                else hero_state <= 2'd0;                // standing

                // decide new obstacle to insert at rightmost (pos2)
                if (gap_count < 3) begin
                    next_new_obst <= EMPTY4;
                    gap_count <= gap_count + 1'b1;
                end else begin
                    // Choose obstacle depending on mode
                    if (hero_small == 1'b0) begin
                        // big hero: only UP, DOWN, or EMPTY
                        case (rand4[1:0])
                            2'b00: begin next_new_obst <= UP4;  gap_count <= 3'd0; end
                            2'b01: begin next_new_obst <= DOWN4;gap_count <= 3'd0; end
                            default: begin next_new_obst <= EMPTY4; gap_count <= gap_count + 1'b1; end
                        endcase
                    end else begin
                        // small hero: more variety (UP, DOWN, OBS3, OBS4, OBS5, EMPTY)
                        // use rand4[2:0] to distribute possibilities
                        case (rand4[2:0])
                            3'd0: begin next_new_obst <= UP4;   gap_count <= 3'd0; end
                            3'd1: begin next_new_obst <= DOWN4; gap_count <= 3'd0; end
                            3'd2: begin next_new_obst <= OBS3;  gap_count <= 3'd0; end
                            3'd3: begin next_new_obst <= OBS4;  gap_count <= 3'd0; end
                            3'd4: begin next_new_obst <= OBS5;  gap_count <= 3'd0; end
                            default: begin next_new_obst <= EMPTY4; gap_count <= gap_count + 1'b1; end
                        endcase
                    end
                end

                // shift obstacles: leftward movement
                pos0 <= pos1;
                pos1 <= pos2;
                pos2 <= next_new_obst;

                // After shifting, check collision at pos0 (which is the obstacle that just arrived at hero's column)
                // Collision detection differs by hero size/mode:

                if (hero_small == 1'b0) begin
                    // BIG HERO (original behavior):
                    // pos0==DOWN  and hero_state != JUMP -> collision
                    // pos0==UP    and hero_state != BEND -> collision
                    if ((pos0 == DOWN4 && hero_state != 2'd1) ||
                        (pos0 == UP4   && hero_state != 2'd2)) begin
                        if (pos0 != EMPTY4) begin
                            lost <= 1'b1;
                        end
                    end
                end else begin
                    // SMALL HERO (new rules confirmed):
                    // hero small has 3 states mapping to positions:
                    //   hero_state==0 -> parado  -> ROM addr 9
                    //   hero_state==1 -> arriba  -> ROM addr 6
                    //   hero_state==2 -> medio   -> ROM addr 8
                    //
                    // Collisions:
                    //  - UP (0): only safe when parado (hero_state==0). If hero in medio or arriba -> crash.
                    //  - DOWN (1): only safe when arriba (hero_state==1).
                    //  - OBS3 (2): safe only when arriba (hero_state==1).
                    //  - OBS4 (11): safe only when medio (hero_state==2).
                    //  - OBS5 (13): safe only when arriba (hero_state==1).
                    if (pos0 != EMPTY4) begin
                        case (pos0)
                            UP4: begin
                                // safe only if parado (hero_state==0)
                                if (!(hero_state == 2'd0)) lost <= 1'b1;
                            end
                            DOWN4: begin
                                // safe only if arriba (hero_state==1)
                                if (!(hero_state == 2'd1)) lost <= 1'b1;
                            end
                            OBS3: begin
                                if (!(hero_state == 2'd1)) lost <= 1'b1; // needs arriba
                            end
                            OBS4: begin
                                if (!(hero_state == 2'd2)) lost <= 1'b1; // needs medio
                            end
                            OBS5: begin
                                if (!(hero_state == 2'd1)) lost <= 1'b1; // needs arriba
                            end
                            default: begin
                                // unknown code -> treat as safe (or ignore)
                            end
                        endcase
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
    // addresses: hero big: standing=5, jump=6, bend=7 ; hero small: parado=9, medio=8, arriba=6
    // obstacles: obst_up=0, obst_down=1, obs3=2, obs4=11, obs5=13, off=15

    always @(*) begin
        // default addresses = off
        addr_left  = 4'd15; // hero display (leftmost)
        addr_ml    = 4'd15; // middle-left (nearest)
        addr_mr    = 4'd15; // middle-right
        addr_right = 4'd15; // rightmost

        if (system_on) begin
            if (lost) begin
                // When lost show standing pose of the current hero for clarity
                if (hero_small == 1'b0) addr_left = 4'd5; // big hero standing
                else addr_left = 4'd9; // small hero parado
            end else begin
                // hero display:
                if (hero_small == 1'b0) begin
                    // BIG hero mapping (original)
                    if (hero_state == 2'd1) addr_left = 4'd6; // hero jump
                    else if (hero_state == 2'd2) addr_left = 4'd7; // hero bend
                    else addr_left = 4'd5; // hero standing
                end else begin
                    // SMALL hero mapping:
                    if (hero_state == 2'd1) addr_left = 4'd6; // arriba
                    else if (hero_state == 2'd2) addr_left = 4'd8; // medio
                    else addr_left = 4'd9; // parado
                end

                // obstacles: pos0 nearest (ml), pos1 (mr), pos2 (rightmost)
                case (pos0)
                    UP4:   addr_ml = 4'd0;
                    DOWN4: addr_ml = 4'd1;
                    OBS3:  addr_ml = 4'd2;
                    OBS4:  addr_ml = 4'd11;
                    OBS5:  addr_ml = 4'd13;
                    default: addr_ml = 4'd15;
                endcase
                case (pos1)
                    UP4:   addr_mr = 4'd0;
                    DOWN4: addr_mr = 4'd1;
                    OBS3:  addr_mr = 4'd2;
                    OBS4:  addr_mr = 4'd11;
                    OBS5:  addr_mr = 4'd13;
                    default: addr_mr = 4'd15;
                endcase
                case (pos2)
                    UP4:   addr_right = 4'd0;
                    DOWN4: addr_right = 4'd1;
                    OBS3:  addr_right = 4'd2;
                    OBS4:  addr_right = 4'd11;
                    OBS5:  addr_right = 4'd13;
                    default: addr_right = 4'd15;
                endcase
            end
        end else begin
            // system off => all OFF (addresses already 15)
            addr_left = 4'd15; addr_ml = 4'd15; addr_mr = 4'd15; addr_right = 4'd15;
        end
    end


    // connect ROM outputs to display scan module in the correct order:
    // We will wire as: dis1 = rightmost, dis2 = middle-right, dis3 = middle-left, dis4 = leftmost(hero)
    display salida_display (
        .clk(clk20M),
        .dis1(rom_seg_ml),    // rightmost
        .dis2(rom_seg_mr),    // middle-right
        .dis3(rom_seg_left),  // middle-left
        .dis4(rom_seg_right), // leftmost (hero)
        .out(display_out),
        .en(enable_out)
    );


    // ---------------------------
    //  Initial values and reset on configuration reload
    // ---------------------------
    initial begin
        pos0 = EMPTY4; pos1 = EMPTY4; pos2 = EMPTY4;
        hero_state = 2'd0;
        gap_count = 3'd3;
        lost = 1'b0;
        system_on = 1'b1;
        hero_small = 1'b0;
        led_lost = 1'b0;
    end

endmodule


