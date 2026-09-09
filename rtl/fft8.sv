// Eight-point FFT for real, signed 16-bit inputs with natural-order outputs. It
// computes two four-point FFTs and combines them with four complex butterflies.
module fft8 (
    input  logic clk, reset, start,
    input  logic signed [15:0] samples [0:7],
    output logic done,
    output logic signed [31:0] real_out [0:7],
    output logic signed [31:0] imag_out [0:7]
);
    // Q1.15 encodings of the only twiddle components needed for an 8-point FFT.
    localparam logic signed [15:0] ONE = 16'sd32767;
    localparam logic signed [15:0] ZERO = 16'sd0;
    localparam logic signed [15:0] SQRT_HALF = 16'sd23170;
    localparam logic signed [15:0] NEG_SQRT_HALF = -16'sd23170;
    localparam logic signed [15:0] NEG_ONE = -16'sd32768;

    logic signed [31:0] x [0:7];
    logic signed [31:0] er [0:3], ei [0:3], orr [0:3], oi [0:3];
    logic signed [31:0] next_real [0:7], next_imag [0:7];

    // Build four-point transforms of the even and odd input indices. The
    // imaginary terms arise from the +/-j rotations in a four-point FFT.
    always_comb begin
        // Widen before arithmetic so intermediate sums cannot overflow at 16 bits.
        x[0] = {{16{samples[0][15]}}, samples[0]};
        x[1] = {{16{samples[1][15]}}, samples[1]};
        x[2] = {{16{samples[2][15]}}, samples[2]};
        x[3] = {{16{samples[3][15]}}, samples[3]};
        x[4] = {{16{samples[4][15]}}, samples[4]};
        x[5] = {{16{samples[5][15]}}, samples[5]};
        x[6] = {{16{samples[6][15]}}, samples[6]};
        x[7] = {{16{samples[7][15]}}, samples[7]};
        er[0] = x[0] + x[2] + x[4] + x[6]; ei[0] = 0;
        er[1] = x[0] - x[4];                   ei[1] = x[6] - x[2];
        er[2] = x[0] - x[2] + x[4] - x[6];    ei[2] = 0;
        er[3] = x[0] - x[4];                   ei[3] = x[2] - x[6];
        orr[0] = x[1] + x[3] + x[5] + x[7];  oi[0] = 0;
        orr[1] = x[1] - x[5];                  oi[1] = x[7] - x[3];
        orr[2] = x[1] - x[3] + x[5] - x[7];  oi[2] = 0;
        orr[3] = x[1] - x[5];                  oi[3] = x[3] - x[7];
    end

    // W8^0 through W8^3 rotate the odd results before combining them with the
    // even results. Each butterfly simultaneously creates bins k and k+4.
    butterfly b0(er[0], ei[0], orr[0], oi[0], ONE, ZERO,
                 next_real[0], next_imag[0], next_real[4], next_imag[4]);
    butterfly b1(er[1], ei[1], orr[1], oi[1], SQRT_HALF, NEG_SQRT_HALF,
                 next_real[1], next_imag[1], next_real[5], next_imag[5]);
    butterfly b2(er[2], ei[2], orr[2], oi[2], ZERO, NEG_ONE,
                 next_real[2], next_imag[2], next_real[6], next_imag[6]);
    butterfly b3(er[3], ei[3], orr[3], oi[3], NEG_SQRT_HALF, NEG_SQRT_HALF,
                 next_real[3], next_imag[3], next_real[7], next_imag[7]);

    integer i;

    // Register all eight combinational results on start. done mirrors start by
    // one registered cycle and marks when the output arrays have been updated.
    always_ff @(posedge clk) begin
        if (reset) begin
            done <= 1'b0;
            for (i = 0; i < 8; i = i + 1) begin
                real_out[i] <= '0;
                imag_out[i] <= '0;
            end
        end else begin
            done <= start;
            if (start) begin
                for (i = 0; i < 8; i = i + 1) begin
                    real_out[i] <= next_real[i];
                    imag_out[i] <= next_imag[i];
                end
            end
        end
    end
endmodule
