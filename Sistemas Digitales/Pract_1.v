// Practica 1 Sistemas Digitales
// By Isaias Matos

module TopModule (
    input C1, input C2, input C3, input C4,
    output V1, output V2, output V3, output V4,
    output H6, output H5, output H4, output H3, output H2, output H1, output H0,
    output T6, output T5, output T4, output T3, output T2, output T1, output T0,
    output U6, output U5, output U4, output U3, output U2, output U1, output U0
);
    
    assign V1 = (~C1 & ~C2 & C3) | (~C1 & C2 & ~C3) | (C1 & ~C2 & ~C3);
    assign V2 = (~C1 & C2 & ~C3 & ~C4) | (C1 & ~C2 & C3 & ~C4) | (~C1 & ~C2 & C4) | (~C2 & ~C3 & C4);
    assign V3 = (~C2 & ~C3 & C4) | (~C2 & C3 & ~C4) | (C1 & C2 & ~C3 & ~C4);
    assign V4 = (~C1 & C2 & C3 & ~C4) | (~C1 & C2 & ~C3 & C4) | (~C1 & ~C2 & C3 & C4);

    assign H6 = (~C1 & ~C3 & ~C4) | (~C2 & ~C3 & ~C4) | (C1 & C2 & C3) | (C1 & C2 & C4) | (C1 & C3 & C4) | (C2 & C3 & C4);
    assign H5 = (C1 & C2) | (~C1 & ~C2 & ~C3) | (~C1 & ~C3 & ~C4) | (~C2 & C3 & ~C4) | (C1 & C4) | (C2 & C3 & C4);
    assign H4 = (C1 & C3) | C4 | (~C1 & C2) | (~C1 & ~C3);
    assign H3 = (C1 & C3 & C4) | (~C1 & ~C3 & ~C4) | (C2 & C3) | (C2 & C4) | (~C1 & C2);
    assign H2 = (~C1 & ~C2 & ~C4) | (C1 & C2) | (C1 & C3 & C4) | (C2 & C3 & C4);
    assign H1 = (~C1 & ~C2 & ~C3 & ~C4) | (C1 & C2 & C3) | (C1 & C2 & C4) | (C3 & C4);
    assign H0 = (C1 & C3 & C4) | (~C1 & ~C3 & ~C4) | (C2 & C3) | (C2 & C4) | (~C1 & C2);
    
    assign T6 = (~C1 & ~C2 & ~C3) | (C1 & C2) | (C1 & C3) | (C2 & C3);
    assign T5 = (~C1 & ~C2 & ~C3 & ~C4) | (C1 & C2 & C3) | (C1 & C2 & C4) | (C1 & C3 & C4) | (C2 & C3 & C4);
    assign T4 = (~C1 & ~C2 & C3) | (C1 & C2 & C3) | (~C1 & ~C3 & ~C4) | (~C2 & ~C3 & ~C4) | (C1 & C4) | (C2 & C4);
    assign T3 = (~C1 & ~C2 & ~C3 & ~C4) | (C1 & C2 & C3) | (C1 & C2 & C4) | (C1 & C3 & C4) | (C2 & C3 & C4);
    assign T2 = (~C1 & ~C2 & ~C3 & ~C4) | (C1 & C2 & C3) | (C1 & C2 & C4) | (C1 & C3 & C4) | (C2 & C3 & C4);
    assign T1 = (~C1 & ~C2 & C3) | (C1 & C2 & C3) | (~C1 & ~C3 & ~C4) | (~C2 & ~C3 & ~C4) | (C1 & C4) | (C2 & C4);
    assign T0 = (~C1 & ~C2 & ~C3 & ~C4) | (C1 & C2 & C3) | (C1 & C2 & C4) | (C1 & C3 & C4) | (C2 & C3 & C4);

    assign U6 = 1'b1;
    assign U5 = (~C1 & ~C2 & ~C3 & ~C4) | (C1 & C2 & C3) | (C1 & C2 & C4) | (C1 & C3 & C4) | (C2 & C3 & C4);
    assign U4 = (~C1 & ~C2 & ~C3 & ~C4) | (C1 & C2 & C3) | (C1 & C2 & C4) | (C1 & C3 & C4) | (C2 & C3 & C4);
    assign U3 = (~C1 & ~C2 & ~C3 & ~C4) | (C1 & C2 & C3) | (C1 & C2 & C4) | (C1 & C3 & C4) | (C2 & C3 & C4);
    assign U2 = (~C1 & ~C2 & ~C3 & ~C4) | (C1 & C2 & C3) | (C1 & C2 & C4) | (C1 & C3 & C4) | (C2 & C3 & C4);
    assign U1 = (~C1 & ~C2 & ~C3 & ~C4) | (C1 & C2 & C3) | (C1 & C2 & C4) | (C1 & C3 & C4) | (C2 & C3 & C4);
    assign U0 = (~C1 & ~C2 & ~C3 & ~C4) | (C1 & C2 & C3) | (C1 & C2 & C4) | (C1 & C3 & C4) | (C2 & C3 & C4);

endmodule

// The End