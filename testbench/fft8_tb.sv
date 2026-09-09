`timescale 1ns/1ps

module fft8_tb;
    logic clk = 0, reset = 1, start = 0, done;
    logic signed [15:0] samples [0:7];
    logic signed [31:0] real_out [0:7], imag_out [0:7];
    logic signed [31:0] expected_real [0:7], expected_imag [0:7];
    integer i, real_error, imag_error;
    integer failures = 0;

    fft8 dut(clk, reset, start, samples, done, real_out, imag_out);

    // A 10 ns period models a 100 MHz clock.
    always #5 clk = ~clk;

    // Load C++-generated vectors, issue one transaction, and compare every
    // complex output. A small tolerance permits fixed-point twiddle error.
    initial begin
        $readmemh("testbench/vectors/input.mem", samples);
        $readmemh("testbench/vectors/expected_real.mem", expected_real);
        $readmemh("testbench/vectors/expected_imag.mem", expected_imag);
        repeat (2) @(posedge clk);
        reset <= 0;
        @(posedge clk); start <= 1;
        @(posedge clk); start <= 0;
        // Wait past nonblocking assignments made on the preceding clock edge.
        #1;

        if (!done) begin
            $display("FAIL: done was not asserted");
            failures = failures + 1;
        end
        for (i = 0; i < 8; i = i + 1) begin
            real_error = real_out[i] - expected_real[i];
            imag_error = imag_out[i] - expected_imag[i];
            if (real_error < 0) real_error = -real_error;
            if (imag_error < 0) imag_error = -imag_error;
            $display("bin %0d: RTL=(%0d,%0d), reference=(%0d,%0d)",
                     i, real_out[i], imag_out[i], expected_real[i], expected_imag[i]);
            if (real_error > 3 || imag_error > 3) begin
                $display("FAIL: bin %0d exceeds tolerance", i);
                failures = failures + 1;
            end
        end
        if (failures == 0) $display("PASS: all bins match the C++ reference");
        else $display("FAIL: %0d checks failed", failures);
        $finish;
    end
endmodule
