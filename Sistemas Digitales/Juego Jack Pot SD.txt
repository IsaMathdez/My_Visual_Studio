

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



// Recibe los 7 segmentos de 3 displays anodo comun.
// Hace el proceso de alternado a alta frecuencia


module display(
    input clk,
    input [6:0] dis1,
    input [6:0] dis2,
    input [6:0] dis3,

    output reg [6:0] out, 
    output reg [2:0] en

);
    always @(posedge clk) 
    case (en)
        3'b001:en = 3'b010;
        3'b010:en = 3'b100;
        3'b100:en = 3'b001; 
        default:en = 3'b001;
    endcase

    always @(posedge clk) 
    begin
    if (en == 3'b001) begin
        out <= dis1;
    end else if (en == 3'b010) begin
        out <= dis2;
    end else if (en == 3'b100) begin
        out <= dis3;
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
    output wire key_start,       // tecla '1'
    output wire key_reset,       // tecla '2'
    output wire key_power        // tecla '3'
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
    assign key_start = (key_valid && key_code == 4'h1);
    assign key_reset = (key_valid && key_code == 4'h4);
    assign key_power = (key_valid && key_code == 4'h7);

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


/*
module LFSR4bit (
    input  wire clk,    // reloj lento (2 Hz)
    input  wire rst,    // reset activo alto
    output reg  [3:0] random  // número pseudoaleatorio (4 bits)
);
    wire feedback;  // bit de realimentación

    // Polinomio x^4 + x^3 + 1 → taps en bits [3] y [2]
    assign feedback = random[3] ^ random[2];

    always @(posedge clk or posedge rst) begin
        if (rst)
            random <= 4'b1111;   // semilla inicial (no cero)
        else
            random <= {random[2:0], feedback}; // desplazamiento con realimentación
    end
endmodule
*/




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
            4'd6  : data = 7'b1000000; // 0
            4'd7  : data = 7'b0111111; // -
            4'd8  : data = 7'b0010010; // 5
            4'd9  : data = 7'b0101010; // M
            4'd10 : data = 7'b0001001; // H 
            4'd11 : data = 7'b0110110; // horizontales
            4'd12 : data = 7'b0001111; // tal
            4'd13 : data = 7'b0111001; // lat
            4'd14 : data = 7'b1011011; // tu ta
            4'd15 : data = 7'b1101101; // ta tu
            
            default: data = 7'b1111111; // apagado

        endcase
    end

endmodule



// Isaias Matos 
// Juego Jack Pot
 

module Jackpot_Game ( // Version 2.5 con on / off
    input  wire clk,          // 100 MHz del FPGA
    input  wire [3:0] row_in,
    output wire [3:0] col_out,
    output wire [6:0] display_out,
    output wire [2:0] enable_out,
    output reg  led_win,
    output wire led_state,
    output wire led_btn
);

    // ---------------------------
    //  Señales internas y clocks
    // ---------------------------
    wire clk0_2, clk0_5, clk1, clk2, clk5, clk10, clk15, clk20, clk20M;
    Clock_divider_multi divs (
        .clock_in(clk),
        .clk_0_2Hz(), .clk_0_5Hz(), .clk_1Hz(clk1), .clk_2Hz(), .clk_5Hz(clk5),
        .clk_10Hz(), .clk_15Hz(), .clk_20Hz(), .clk_20MHz(clk20M)
    );

    // ---------------------------
    //  Señales del keypad
    // ---------------------------
    wire [3:0] key_code;
    wire key_valid;
    wire key_start, key_reset, key_power;

    // === Instancia del Keypad 4x4 ===
    keypad_4x4_controller u_keypad (
        .clk(clk20M), // clk rapido
        .rst_n(),   // vacio
        .row(row_in),
        .col(col_out),
        .key_code(key_code),
        .key_valid(key_valid),
        .key_start(key_start), // estas son mis salidas personalizadas
        .key_reset(key_reset),
        .key_power(key_power)
    );

    // Estas señales hay que invertirlas para el debouncer
    assign btn_start  = ~key_start;
    assign btn_reset  = ~key_reset;
    assign btn_power  = ~key_power;

    // ---------------------------
    //  Debounce de botones
    // ---------------------------
    wire pb_start_state;
    wire pb_reset_state;
    wire pb_power_state;

    PushButton_Debouncer db_start (
        .clk(clk20M), .PB(btn_start),
        .PB_state(pb_start_state),
        .PB_down(),
        .PB_up()
    );

    PushButton_Debouncer db_reset (
        .clk(clk20M), .PB(btn_reset),
        .PB_state(pb_reset_state),
        .PB_down(),
        .PB_up()
    );
    
    PushButton_Debouncer db_power (
        .clk(clk20M), .PB(btn_power),
        .PB_state(pb_power_state),
        .PB_down(),
        .PB_up()
    );

    // Comprobacion de botones del keypad
    assign led_btn = pb_start_state | pb_power_state | pb_reset_state;

    
    // ==========================================
    // Sistema ON / OFF retentivo
    // ==========================================

    reg system_on = 1'b1;       // sistema inicia encendido
    reg pb_power_prev = 1'b0;   // para detectar flanco de subida

    always @(posedge clk20M or posedge pb_reset_state) begin
        if (pb_reset_state) begin
            system_on <= 1'b1;       // al reset, sistema encendido
            pb_power_prev <= 1'b0;
        end else begin
            pb_power_prev <= pb_power_state;

            // Detectar flanco de subida del botón ON/OFF
            if (~pb_power_prev && pb_power_state)
                system_on <= ~system_on;   // invierte el estado
        end
    end

    // LED indicador de apagado (ON cuando el sistema está apagado)
    assign led_state = ~system_on;

    // reset general activo high
    wire top_reset = pb_reset_state | (~system_on);


    // ---------------------------
    //  Generador aleatorio
    // ---------------------------
    wire [3:0] rand_val;
    RandomMix rnd (
        .clk(system_on ? clk5 : 1'b0),
        .rst(top_reset),
        .random_out(rand_val)
    );

    // ---------------------------
    //  ROM de patrones 7 segmentos
    // ---------------------------
    wire [6:0] rom_data;
    ROM_7seg rom (
        .addr(rand_val),
        .data(rom_data)
    );

    // ---------------------------
    //  FSM del juego
    // ---------------------------
    reg [2:0] state = 0;
    localparam IDLE   = 3'd0,
               RUN_L  = 3'd1,
               RUN_M  = 3'd2,
               RUN_R  = 3'd3,
               CHECK  = 3'd4,
               WIN    = 3'd5;

    reg [2:0] timer; // contador de segundos (0–5)
    reg [6:0] val_L, val_M, val_R; // valores fijos al final
    reg [6:0] seg_left, seg_mid, seg_right;

    always @(posedge clk1 or posedge top_reset) begin
        if (top_reset) begin
            state  <= IDLE;
            timer  <= 0;
            seg_left  <= 7'b0111111;
            seg_mid   <= 7'b0111111;
            seg_right <= 7'b0111111;
            led_win   <= 0;
        end else begin
            case (state)
                IDLE: begin
                    led_win <= 0;
                    if (pb_start_state) begin
                        state <= RUN_L;
                        timer <= 0;
                    end
                end

                // --- DISPLAY IZQUIERDO ---
                RUN_L: begin
                    seg_left <= rom_data;
                    timer <= timer + 1;
                    if (timer == 5) begin
                        val_L <= rom_data;
                        state <= RUN_M;
                        timer <= 0;
                    end
                end

                // --- DISPLAY MEDIO ---
                RUN_M: begin
                    seg_mid <= rom_data;
                    timer <= timer + 1;
                    if (timer == 5) begin
                        val_M <= rom_data;
                        state <= RUN_R;
                        timer <= 0;
                    end
                end

                // --- DISPLAY DERECHO ---
                RUN_R: begin
                    seg_right <= rom_data;
                    timer <= timer + 1;
                    if (timer == 5) begin
                        val_R <= rom_data;
                        state <= CHECK;
                        timer <= 0;
                    end
                end

                // --- VERIFICAR GANADOR ---
                CHECK: begin
                    if ((val_L == val_M && val_M == val_R)||(val_L == val_M)||(val_L == val_R)||(val_R == val_M))
                        state <= WIN;
                    else
                        state <= IDLE; // ojo
                end

                // --- GANASTE ---
                WIN: begin
                    led_win <= ~led_win; // parpadea
                end
            endcase
        end
    end

    // -------------------------
    // Conexión a la placa:
    // 'display' es el módulo que hace scan físico (usa clk20M).
    // -------------------------
    wire [6:0] display_out_prev;
    wire [2:0] enable_out_prev;

    display salida_display (
        .clk(clk20M),
        .dis1(seg_mid),
        .dis2(seg_left),
        .dis3(seg_right),
        .out(display_out_prev),
        .en(enable_out_prev)
    );

    
    // Displays apagados si el sistema está OFF
    assign display_out = system_on ? display_out_prev : 7'b1111111;
    assign enable_out  = system_on ? enable_out_prev : 3'b000;
    

endmodule



