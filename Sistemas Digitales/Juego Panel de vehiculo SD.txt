
// Hecho por Isaias Matos

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
    output reg clk_50Hz,
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
    parameter DIV_50Hz  = FREQ_IN / 50;
    parameter DIV_20MHz  = FREQ_IN / 20_000_000;

    // Contadores
    reg [31:0] cnt_0_2Hz = 0;
    reg [31:0] cnt_0_5Hz   = 0;
    reg [31:0] cnt_1Hz   = 0;
    reg [31:0] cnt_2Hz  = 0;
    reg [31:0] cnt_5Hz  = 0;
    reg [31:0] cnt_10Hz  = 0;
    reg [31:0] cnt_15Hz  = 0;
    reg [31:0] cnt_50Hz  = 0;
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

        // --- 50 Hz ---
        if (cnt_50Hz >= (DIV_50Hz-1)) 
            cnt_50Hz <= 0;
        else 
            cnt_50Hz <= cnt_50Hz + 1;
            clk_50Hz <= (cnt_50Hz < DIV_50Hz/2);

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





// ---------------------- Double Dabble ----------------------
module FA1 (input A, B, Cin, output Sum, Cout);
    assign Sum = A ^ B ^ Cin;
    assign Cout = (A & B) | (A & Cin) | (B & Cin);

endmodule

module Adder4 (input [3:0] A, B, input Cin, output [3:0] Sum, 
output Cout);
    wire c1, c2, c3;
    FA1 fa0(.A(A[0]), .B(B[0]), .Cin(Cin), .Sum(Sum[0]), 
    .Cout(c1));
    FA1 fa1(.A(A[1]), .B(B[1]), .Cin(c1), .Sum(Sum[1]), 
    .Cout(c2));
    FA1 fa2(.A(A[2]), .B(B[2]), .Cin(c2), .Sum(Sum[2]), 
    .Cout(c3));
    FA1 fa3(.A(A[3]), .B(B[3]), .Cin(c3), .Sum(Sum[3]), 
    .Cout(Cout));

endmodule

module GE5 (input [3:0] in, output ge5);
    assign ge5 = in[3] | (in[2] & (in[1] | in[0]));

endmodule

module Add3_mux4 (input [3:0] in, output [3:0] out);
    wire ge5_flag;
    wire [3:0] sum3;
    wire carry;
    GE5 u_ge5(.in(in), .ge5(ge5_flag));
    Adder4 u_add3(.A(in), .B(4'b0011), .Cin(1'b0), .Sum(sum3), 
    .Cout(carry));
    assign out = ge5_flag ? sum3 : in;

endmodule

module DblDblStage (input [19:0] bus_in, output [19:0] bus_out);
    wire [3:0] H_in = bus_in[19:16];
    wire [3:0] T_in = bus_in[15:12];
    wire [3:0] U_in = bus_in[11:8];
    wire [7:0] R_in = bus_in[7:0];
    wire [3:0] H_adj, T_adj, U_adj;
    Add3_mux4 u_adj_H(.in(H_in), .out(H_adj));
    Add3_mux4 u_adj_T(.in(T_in), .out(T_adj));
    Add3_mux4 u_adj_U(.in(U_in), .out(U_adj));
    wire [19:0] prebus = {H_adj, T_adj, U_adj, R_in};
    assign bus_out = {prebus[18:0], 1'b0};

endmodule

module DoubleDabble8_BCD (
    input wire [7:0] bin,
    output wire [3:0] H,
    output wire [3:0] T,
    output wire [3:0] U
    );

 // estructura del DoubleDabble: 8 etapas de DblDblStage
    wire [19:0] stage_bus [0:8];
    assign stage_bus[0] = {4'h0,4'h0,4'h0,bin};
    genvar gi;
    generate for (gi=1; gi<=8; gi=gi+1) begin : gen_stages
        DblDblStage u_stage(.bus_in(stage_bus[gi-1]), 
        .bus_out(stage_bus[gi]));
    end endgenerate

    assign H = stage_bus[8][19:16];
    assign T = stage_bus[8][15:12];
    assign U = stage_bus[8][11:8];

endmodule




module BCDto7Segment ( // anodo comun
    input  [3:0] bcd,
    output reg [6:0] seg  // 
);
    always @(*) begin
        case (bcd)
            4'd0: seg = 7'b1000000;
            4'd1: seg = 7'b1111001;
            4'd2: seg = 7'b0100100;
            4'd3: seg = 7'b0110000;
            4'd4: seg = 7'b0011001;
            4'd5: seg = 7'b0010010;
            4'd6: seg = 7'b0000010;
            4'd7: seg = 7'b1111000;
            4'd8: seg = 7'b0000000;
            4'd9: seg = 7'b0010000;
            default: seg = 7'b1111111; // apagado
        endcase
    end
endmodule




// Multiplexado para 4 displays anodo comun

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




// ROM combinacional de 64 x 7 bits

module ROM_7seg (
    input  wire [5:0] addr,
    output reg  [6:0] data
);

    always @(*) begin
        case (addr)
            // 0: display apagado (todos segmentos apagados -> 1)
            6'd0  : data = 7'b1111111; // OFF

            // 1..10: dígitos 0..9  (gfedcba, activo bajo)
            6'd1  : data = 7'b1000000; // 0 -> a b c d e f
            6'd2  : data = 7'b1111001; // 1 -> b c
            6'd3  : data = 7'b0100100; // 2 -> a b d e g
            6'd4  : data = 7'b0110000; // 3 -> a b c d g
            6'd5  : data = 7'b0011001; // 4 -> b c f g
            6'd6  : data = 7'b0010010; // 5 -> a c d f g
            6'd7  : data = 7'b0000010; // 6 -> a c d e f g
            6'd8  : data = 7'b1111000; // 7 -> a b c
            6'd9  : data = 7'b0000000; // 8 -> a b c d e f g
            6'd10 : data = 7'b0010000; // 9 -> a b c d f g

            // 11..36: letras A..Z (representaciones convencionales en 7-seg)
            6'd11 : data = 7'b0001000; // A ->
            6'd12 : data = 7'b0000011; // B -> 
            6'd13 : data = 7'b1000110; // C -> 
            6'd14 : data = 7'b0100001; // D -> 
            6'd15 : data = 7'b0000110; // E -> 
            6'd16 : data = 7'b0001110; // F -> 
            6'd17 : data = 7'b0010000; // G -> 
            6'd18 : data = 7'b0001001; // H -> 
            6'd19 : data = 7'b1111001; // I -> 
            6'd20 : data = 7'b1110001; // J ->
            6'd21 : data = 7'b0001111; // K -> 
            6'd22 : data = 7'b1000111; // L -> 
            6'd23 : data = 7'b1101010; // M ->  
            6'd24 : data = 7'b1001000; // N -> 
            6'd25 : data = 7'b1000000; // O -> 
            6'd26 : data = 7'b0001100; // P ->
            6'd27 : data = 7'b0011000; // Q -> 
            6'd28 : data = 7'b1001110; // R -> 
            6'd29 : data = 7'b0010010; // S -> 
            6'd30 : data = 7'b0000111; // T -> 
            6'd31 : data = 7'b1000001; // U -> 
            6'd32 : data = 7'b1100011; // V -> 
            6'd33 : data = 7'b1010101; // W -> 
            6'd34 : data = 7'b0111001; // X -> 
            6'd35 : data = 7'b0010001; // Y -> 
            6'd36 : data = 7'b0100100; // Z -> 

            // 37..40: simbolos cuatro direcciones
            6'd37  : data = 7'b1111001; // | der
            6'd38  : data = 7'b1001111; // | izq
            6'd39  : data = 7'b1111110; // -
            6'd40  : data = 7'b1110111; // _

            // 41..63: resto -> solo segmento 'g' encendido
            // segmento g encendido => g = 0, resto = 1 
            default: data = 7'b0111111; // -
        endcase
    end

endmodule




// Isaias Matos
// Sistema digital Vehiculo


module Car_pad (   // Version 2.5
    // LISTOOO
    input  wire clk,           // 100 MHz del FPGA
    input  wire [3:0] row_in,
    input  wire dip_seatbelt_left,   // dip switch seatbelt left (1 = fastened)
    input  wire dip_seatbelt_right,  // dip switch seatbelt right
    input  wire dip_fill,       // dip switch: when 1 => start filling tank (only works in P)
    output wire [3:0] col_out,
    output wire [6:0] display_out,
    output wire [3:0] enable_out,
    output wire led_fuel_1,
    output wire led_fuel_2,
    output wire led_fuel_3,
    output wire led_fuel_4,
    output wire led_headlights1, led_headlights2,
    output wire led_rear_lights1, led_rear_lights2,
    output wire buzzer,
    output wire led_out
);

    // ---------------------------
    //  Clocks / divisores
    // ---------------------------
    wire clk0_2, clk0_5, clk1, clk2, clk5, clk10, clk15, clk50, clk20M;
    Clock_divider_multi divs (
        .clock_in(clk),
        .clk_0_2Hz(clk0_2), .clk_0_5Hz(clk0_5), .clk_1Hz(clk1), .clk_2Hz(clk2), .clk_5Hz(clk5),
        .clk_10Hz(clk10), .clk_15Hz(clk15), .clk_50Hz(clk50), .clk_20MHz(clk20M)
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
    wire key_parking      = (key_valid && key_code == 4'hA); // A
    wire key_neutral      = (key_valid && key_code == 4'hB); // B
    wire key_reverse      = (key_valid && key_code == 4'hC); // C
    wire key_drive        = (key_valid && key_code == 4'hD); // D

    wire key_door_left    = (key_valid && key_code == 4'h1); // 1
    wire key_door_right   = (key_valid && key_code == 4'h4); // 4
    wire key_bonnet       = (key_valid && key_code == 4'h7); // 7
    wire key_trunk        = (key_valid && key_code == 4'hE); // *

    wire key_accel        = (key_valid && key_code == 4'h2); // 2
    wire key_brake        = (key_valid && key_code == 4'h5); // 5

    wire key_hazard       = (key_valid && key_code == 4'h8); // 8
    wire key_headlights   = (key_valid && key_code == 4'h0); // 0

    wire key_mirror_sel   = (key_valid && key_code == 4'h3); // 3
    wire key_mirror_step  = (key_valid && key_code == 4'h6); // 6

    wire key_power_kp     = (key_valid && key_code == 4'hF); // #
    wire key_unused       = (key_valid && key_code == 4'h9); // 9  (libre)

    // ---------------------------
    //  Invertimos para obtener señales tipo PB (activo LOW) para el debouncer
    // ---------------------------
    wire btn_parking    = ~key_parking;
    wire btn_neutral    = ~key_neutral;
    wire btn_reverse    = ~key_reverse;
    wire btn_drive      = ~key_drive;

    wire btn_door_left  = ~key_door_left;
    wire btn_door_right = ~key_door_right;
    wire btn_bonnet     = ~key_bonnet;
    wire btn_trunk      = ~key_trunk;

    wire btn_accel      = ~key_accel;
    wire btn_brake      = ~key_brake;

    wire btn_hazard     = ~key_hazard;
    wire btn_headlights = ~key_headlights;

    wire btn_mirror_sel = ~key_mirror_sel;
    wire btn_mirror_step= ~key_mirror_step;

    wire btn_power_kp_n = ~key_power_kp;

    // ---------------------------
    //  Debouncers: cada btn_* -> pb_*_state (PB_state: active HIGH)
    // ---------------------------
    wire pb_parking_state, pb_neutral_state, pb_reverse_state, pb_drive_state;
    wire pb_door_left_state, pb_door_right_state, pb_bonnet_state, pb_trunk_state;
    wire pb_accel_state, pb_brake_state;
    wire pb_hazard_state, pb_headlights_state;
    wire pb_mirror_sel_state, pb_mirror_step_state, pb_power_kp_state;

    PushButton_Debouncer db_parking (
        .clk(clk20M), .PB(btn_parking),
        .PB_state(pb_parking_state), .PB_down(), .PB_up()
    );
    PushButton_Debouncer db_neutral (
        .clk(clk20M), .PB(btn_neutral),
        .PB_state(pb_neutral_state), .PB_down(), .PB_up()
    );
    PushButton_Debouncer db_reverse (
        .clk(clk20M), .PB(btn_reverse),
        .PB_state(pb_reverse_state), .PB_down(), .PB_up()
    );
    PushButton_Debouncer db_drive (
        .clk(clk20M), .PB(btn_drive),
        .PB_state(pb_drive_state), .PB_down(), .PB_up()
    );

    PushButton_Debouncer db_leftdoor (
        .clk(clk20M), .PB(btn_door_left),
        .PB_state(pb_door_left_state), .PB_down(), .PB_up()
    );
    PushButton_Debouncer db_rightdoor (
        .clk(clk20M), .PB(btn_door_right),
        .PB_state(pb_door_right_state), .PB_down(), .PB_up()
    );
    PushButton_Debouncer db_bonnet (
        .clk(clk20M), .PB(btn_bonnet),
        .PB_state(pb_bonnet_state), .PB_down(), .PB_up()
    );
    PushButton_Debouncer db_trunk (
        .clk(clk20M), .PB(btn_trunk),
        .PB_state(pb_trunk_state), .PB_down(), .PB_up()
    );

    PushButton_Debouncer db_accel (
        .clk(clk20M), .PB(btn_accel),
        .PB_state(pb_accel_state), .PB_down(), .PB_up()
    );
    PushButton_Debouncer db_brake (
        .clk(clk20M), .PB(btn_brake),
        .PB_state(pb_brake_state), .PB_down(), .PB_up()
    );

    PushButton_Debouncer db_hazard (
        .clk(clk20M), .PB(btn_hazard),
        .PB_state(pb_hazard_state), .PB_down(), .PB_up()
    );
    PushButton_Debouncer db_headlights (
        .clk(clk20M), .PB(btn_headlights),
        .PB_state(pb_headlights_state), .PB_down(), .PB_up()
    );

    PushButton_Debouncer db_mirror_sel (
        .clk(clk20M), .PB(btn_mirror_sel),
        .PB_state(pb_mirror_sel_state), .PB_down(), .PB_up()
    );
    PushButton_Debouncer db_mirror_step (
        .clk(clk20M), .PB(btn_mirror_step),
        .PB_state(pb_mirror_step_state), .PB_down(), .PB_up()
    );

    PushButton_Debouncer db_powerkp (
        .clk(clk20M), .PB(btn_power_kp_n),
        .PB_state(pb_power_kp_state), .PB_down(), .PB_up()
    );

    // simple debug led: any pb_state high
    /*assign led_out = pb_parking_state | pb_neutral_state | pb_reverse_state | pb_drive_state | pb_door_left_state
     | pb_door_right_state | pb_bonnet_state | pb_trunk_state | pb_accel_state | pb_brake_state | pb_hazard_state
     | pb_headlights_state | pb_power_kp_state; */



    // ===========================
    //  FSM DE ESTADOS DEL VEHÍCULO
    // ===========================
    localparam [1:0]
        ST_P = 2'b00,   // Parking
        ST_N = 2'b01,   // Neutral
        ST_D = 2'b10,   // Drive
        ST_R = 2'b11;   // Reverse

    reg [1:0] state, next_state;

    // Buttons mapped (use debounced pb_*_state to change states)
    wire pb_P_state = pb_parking_state;    // Parking
    wire pb_N_state = pb_neutral_state;    // Neutral
    wire pb_D_state = pb_drive_state;      // Drive
    wire pb_R_state = pb_reverse_state;    // Reverse

    // system_on: retentive toggle — detect edge in clk20M domain
    reg system_on = 1'b0;
    reg pb_power_prev = 1'b0;

    always @(posedge clk20M) begin
        pb_power_prev <= pb_power_kp_state;
        if (~pb_power_prev & pb_power_kp_state) begin
            system_on <= ~system_on;
        end
    end

    // FSM sequential: update at clk1 but only advance when system_on
    always @(posedge clk20M) begin
        if (!system_on) begin
            state <= ST_P;  // siempre vuelve a Parking cuando el sistema está apagado
        end 
        else begin
            state <= next_state;  // si el sistema está encendido, sigue su FSM normal
        end
    end


    // combinational transitions
    always @(*) begin
        next_state = state;
        case (state)
            default: 
                         if (pb_D_state) next_state = ST_D;
                         else if (pb_P_state) next_state = ST_P;
                         else if (pb_R_state) next_state = ST_R;
                         else if (pb_N_state) next_state = ST_N;
                     
        endcase
    end


    // ===========================
    //  Press-lock between ACCEL and BRAKE
    //  press_lock: 0 = none, 1 = accel, 2 = brake
    //  We implement in clk20M domain sampling pb_accel_state/pb_brake_state
    // ===========================
    reg [1:0] press_lock = 2'b00;
    always @(posedge clk20M) begin
        // if no lock, take first pressed
        if (press_lock == 2'd0) begin
            if (pb_accel_state) press_lock <= 2'd1;
            else if (pb_brake_state) press_lock <= 2'd2;
        end else if (press_lock == 2'd1) begin
            // accel locked: release when accel released
            if (~pb_accel_state) press_lock <= 2'd0;
        end else if (press_lock == 2'd2) begin
            if (~pb_brake_state) press_lock <= 2'd0;
        end
    end


    // ===========================
    //  Synchronize slower clocks into clk20M domain and detect rising edges
    //  We will generate one-cycle pulses in clk20M domain: tick_clk2, tick_clk5, tick_clk0_5, tick_clk1, tick_clk10
    // ===========================
    reg s_clk2_0, s_clk2_1;
    reg s_clk5_0, s_clk5_1;
    reg s_clk0_5_0, s_clk0_5_1;
    reg s_clk1_0, s_clk1_1;
    reg s_clk10_0, s_clk10_1;

    reg tick_clk2, tick_clk5, tick_clk0_5, tick_clk1_pulse, tick_clk10;

    always @(posedge clk20M) begin
        // sample
        s_clk2_0   <= clk2;     s_clk2_1   <= s_clk2_0;
        s_clk5_0   <= clk5;     s_clk5_1   <= s_clk5_0;
        s_clk0_5_0 <= clk0_5;   s_clk0_5_1 <= s_clk0_5_0;
        s_clk1_0   <= clk1;     s_clk1_1   <= s_clk1_0;
        s_clk10_0  <= clk10;    s_clk10_1  <= s_clk10_0;

        // edge detect (rising)
        tick_clk2    <= (s_clk2_0   & ~s_clk2_1);
        tick_clk5    <= (s_clk5_0   & ~s_clk5_1);
        tick_clk0_5  <= (s_clk0_5_0 & ~s_clk0_5_1);
        tick_clk1_pulse <= (s_clk1_0 & ~s_clk1_1);
        tick_clk10   <= (s_clk10_0  & ~s_clk10_1);
    end


    // ===========================
    //  Speed and Fuel counters (single driver in clk20M domain reacting to ticks)
    //  speed: 0..99  (8-bit)
    //  fuel:  0..240 (8-bit) where 240 = full
    // ===========================
    reg [7:0] speed = 8'd0;
    reg [7:0] fuel  = 8'd240;
    reg fuel_empty = 1'b0;

    // We'll update speed/fuel on clk20M when tick signals are asserted
    always @(posedge clk20M) begin
        // speed behavior
        // if system off -> freeze speed to 0
        if (!system_on) begin
            speed <= 8'd0;
        end else begin
            // immediate zero when in Parking
            if (next_state == ST_P) begin
                speed <= 8'd0;
            end else begin
                // accelerate at 5 Hz ticks 
                if (tick_clk5) begin
                    if ((state == ST_D || state == ST_R) && (press_lock == 2'd1) && (fuel > 0)) begin
                        if (speed < 8'd99) speed <= speed + 1;
                    end
                end

                // brake decrease at 10 Hz
                if (tick_clk10) begin
                    if ((state == ST_D || state == ST_R) && (press_lock == 2'd2)) begin
                        if (speed > 0) speed <= speed - 1;
                    end
                end

                // natural decay at 0.5 Hz (when none pressed OR in N)
                if (tick_clk0_5) begin
                    if ((state == ST_D || state == ST_R) && (press_lock == 2'd0)) begin
                        if (speed > 0) speed <= speed - 1;
                    end else if (state == ST_N) begin
                        if (speed > 0) speed <= speed - 1;
                    end
                end
            end
        end

        // fuel behavior (1 Hz consumption while accel in D/R; fill at 10Hz in P)
        if (!system_on) begin
            fuel <= fuel;
            fuel_empty <= fuel_empty;
        end else begin
            // filling at 10Hz when dip_fill active and in Parking
            if (tick_clk10 && dip_fill && (state == ST_P)) begin
                if (fuel < 8'd240) fuel <= fuel + 1;
            end

            // consumption at 1Hz when accelerating in D or R and press_lock == accel
            if (tick_clk1_pulse) begin
                if ((state == ST_D || state == ST_R || state == ST_N) && (press_lock == 2'd1) && (fuel > 0)) begin
                    fuel <= fuel - 1;
                end 
            end

            // update fuel_empty flag
            if (fuel == 8'd0) fuel_empty <= 1'b1;
            else fuel_empty <= 1'b0;
        end
    end

    // Leds indican nivel de gasolina 
    assign led_fuel_1 = (system_on ? (fuel > 8'd0) : 1'b0); // agrege
    assign led_fuel_2 = (system_on ? (fuel >= 8'd60): 1'b0);
    assign led_fuel_3 = (system_on ? (fuel >= 8'd120): 1'b0);
    assign led_fuel_4 = (system_on ? (fuel >= 8'd180): 1'b0); 

    // ---------------------------
    // BOTONES DE LUCES (toggles) (use pb_headlights_state, pb_hazard_state)
    // ---------------------------
    reg headlights_on = 1'b0;
    reg hazard_on     = 1'b0;

    // We toggle on rising edge of pb_* sampled at clk20M to avoid multi-toggle while pressed
    reg ph_head_prev = 1'b0, ph_hazard_prev = 1'b0;
    always @(posedge clk20M) begin
        ph_head_prev <= pb_headlights_state;
        ph_hazard_prev <= pb_hazard_state;
        if (~ph_head_prev & pb_headlights_state) headlights_on <= ~headlights_on;
        if (~ph_hazard_prev & pb_hazard_state) hazard_on <= ~hazard_on;
    end

    // ---------------------------
    // LUCES OUTPUTS
    // ---------------------------
    assign led_headlights1 = (system_on && (hazard_on ? blink1 : headlights_on));
    assign led_headlights2 = (system_on && (hazard_on ? blink1 : headlights_on)); // agrege

    // hazard: blink at 1 Hz (clk1) when hazard_on
    // implement blink by sampling clk1 in clk20M domain (we already have tick_clk1_pulse)
    // create a blink_reg toggled on each clk1 edge
    reg blink1 = 1'b0;
    always @(posedge clk20M) begin
        if (tick_clk1_pulse) blink1 <= ~blink1;
    end

    // rear lights when brake pressed
    assign led_rear_lights1 = (system_on && (hazard_on ? blink1 : (pb_brake_state && (state == ST_D || state == ST_R))));
    assign led_rear_lights2 = (system_on && (hazard_on ? blink1 : (pb_brake_state && (state == ST_D || state == ST_R)))); // agrege

    // ---------------------------
    //  PUERTAS: toggles (already present in your code)
    // ---------------------------
    reg door_left_open  = 1'b0;
    reg door_right_open = 1'b0;
    reg hood_open       = 1'b0;
    reg trunk_open      = 1'b0;

    // toggle doors on rising edge of pb_* sampled in clk20M
    reg pdl_prev=1'b0, pdr_prev=1'b0, pbn_prev=1'b0, ptr_prev=1'b0;
    always @(posedge clk20M) begin
        pdl_prev <= pb_door_left_state;
        pdr_prev <= pb_door_right_state;
        pbn_prev <= pb_bonnet_state;
        ptr_prev <= pb_trunk_state;

        if (~pdl_prev & pb_door_left_state)  door_left_open  <= ~door_left_open;
        if (~pdr_prev & pb_door_right_state) door_right_open <= ~door_right_open;
        if (~pbn_prev & pb_bonnet_state)     hood_open       <= ~hood_open;
        if (~ptr_prev & pb_trunk_state)      trunk_open      <= ~trunk_open;
    end

    // any_door_open, used by buzzer logic below
    wire any_door_open = door_left_open | door_right_open | hood_open | trunk_open;

    // ---------------------------
    //  BUZZER LOGIC
    //  - buzzer to 1Hz when: in N/D/R AND (any_door_open OR seatbelt not fastened)
    //  - also 1Hz when low fuel (>0 && <=60)
    //  - if fuel_empty -> use faster indicator (clk5)
    // ---------------------------
    wire seatbelt_left_ok  = dip_seatbelt_left;
    wire seatbelt_right_ok = dip_seatbelt_right;
    wire low_fuel = (fuel <= 8'd60 && fuel > 0);

    // create buzzer_out_reg: 0 or 1 toggling at 1Hz when needed; if fuel_empty use clk5
    reg buzzer_reg = 1'b0;
    always @(posedge clk20M) begin
        // toggle buzzer_reg on each clk1 edge if condition true
        if (((state == ST_N) || (state == ST_D) || (state == ST_R)) &&
            (any_door_open || ~seatbelt_left_ok || ~seatbelt_right_ok)) begin
            if (tick_clk1_pulse) buzzer_reg <= ~buzzer_reg;
        end else if (low_fuel) begin
            if (tick_clk1_pulse) buzzer_reg <= ~buzzer_reg;
        end else if (fuel == 8'd0) begin
            if (tick_clk5) buzzer_reg <= ~buzzer_reg; // faster blink when empty
        end else begin
            buzzer_reg <= 1'b0;
        end
    end
    assign buzzer = buzzer_reg;
    assign led_out = (system_on ? buzzer : 1'b0); // agrege


    // ===========================
    //  CONEXIÓN CON DISPLAYS
    // ===========================
    reg [5:0] rom_addr_display1;
    wire [6:0] seg_display1;
    wire [6:0] prev_seg_display1;
    wire [6:0] prev_seg_display2;
    wire [6:0] prev_seg_display3;
    wire [6:0] seg_display2;
    wire [6:0] seg_display3;
    wire [6:0] seg_display4;

    // Display 1: letra del estado  (kept your mapping numbers)
    always @(*) begin
        case (state)
            ST_P: rom_addr_display1 = 6'd26;  // 'P' 
            ST_N: rom_addr_display1 = 6'd24;  // 'N'
            ST_D: rom_addr_display1 = 6'd14;  // 'D'
            ST_R: rom_addr_display1 = 6'd28;  // 'R'
            default: rom_addr_display1 = 6'd0;
        endcase
    end

    ROM_7seg rom_display1 (
        .addr(rom_addr_display1),
        .data(prev_seg_display1)
    );

    assign seg_display1 = (system_on ? prev_seg_display1 : 7'b1111111); // agrege

    // Display 2 y 3: velocímetro bin->BCD->7seg
    wire [3:0] bcd_tens, bcd_units;

    DoubleDabble8_BCD conv_bcd (
        .bin(speed),
        .H(), .T(bcd_tens), .U(bcd_units)
    );

    BCDto7Segment disp2 (.bcd(bcd_tens),  .seg(prev_seg_display2));
    BCDto7Segment disp3 (.bcd(bcd_units), .seg(prev_seg_display3));

    assign seg_display2 = (system_on ? prev_seg_display2 : 7'b1111111); // agrege esto
    assign seg_display3 = (system_on ? prev_seg_display3 : 7'b1111111);

    // Display 4: Puertas / baúl / capó
    wire [6:0] seg_leftdoor;
    wire [6:0] seg_rightdoor;
    wire [6:0] seg_hood;
    wire [6:0] seg_trunk;

    wire [5:0] address1 = (door_left_open  ? 6'd38 : 6'd0);
    wire [5:0] address2 = (door_right_open ? 6'd37 : 6'd0);
    wire [5:0] address3 = (hood_open       ? 6'd39 : 6'd0);
    wire [5:0] address4 = (trunk_open      ? 6'd40 : 6'd0);

    ROM_7seg rom_leftdoor  (.addr(address1), .data(seg_leftdoor));
    ROM_7seg rom_rightdoor (.addr(address2), .data(seg_rightdoor));
    ROM_7seg rom_hood      (.addr(address3), .data(seg_hood));
    ROM_7seg rom_trunk     (.addr(address4), .data(seg_trunk));

    // Concatenacion
    assign seg_display4 = (system_on ? {1'b1, seg_leftdoor[5], 1'b1, seg_trunk[3], 1'b1, seg_rightdoor[1], seg_hood[0]} : 7'b1111111); // agrege


    // display multiplexer: dis1=rightmost, dis2=middle-right, dis3=middle-left, dis4=leftmost(hero)
    display salida_display (
        .clk(clk20M),
        .dis1(seg_display3),
        .dis2(seg_display2),
        .dis3(seg_display1), 
        .dis4(seg_display4),
        .out(display_out),
        .en(enable_out)
    ); 

endmodule


