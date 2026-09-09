// Radix-2 complex butterfly implementing A + W*B and A - W*B. Signal values
// are signed integers, while the real and imaginary twiddle inputs use Q1.15.
module butterfly (
    input  logic signed [31:0] a_real, a_imag, b_real, b_imag,
    input  logic signed [15:0] w_real, w_imag,
    output logic signed [31:0] y0_real, y0_imag, y1_real, y1_imag
);
    logic signed [47:0] product_rr, product_ii, product_ri, product_ir;
    logic signed [48:0] product_real, product_imag;
    logic signed [31:0] rotated_real, rotated_imag;

    // Expand complex multiplication into four real multiplications. The 48-bit
    // products retain the full precision of a 32-bit signal times a 16-bit twiddle.
    always_comb begin
        product_rr = b_real * w_real;
        product_ii = b_imag * w_imag;
        product_ri = b_real * w_imag;
        product_ir = b_imag * w_real;
        product_real = product_rr - product_ii;
        product_imag = product_ri + product_ir;
        // Remove the Q1.15 fractional bits; >>> preserves the sign for negatives.
        rotated_real = product_real >>> 15;
        rotated_imag = product_imag >>> 15;
        y0_real = a_real + rotated_real;
        y0_imag = a_imag + rotated_imag;
        y1_real = a_real - rotated_real;
        y1_imag = a_imag - rotated_imag;
    end
endmodule
