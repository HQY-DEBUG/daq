# 输入时钟约束（50MHz，周期 20ns）
# create_clock -period 20.000 -name pl_clk [get_ports pl_clk]

# clk_wiz_0 生成的 64MHz 时钟约束（周期 15.625ns）
# create_generated_clock -name clk_64m -source [get_pins u_clk/clk_in1] -divide_by 10 -multiply_by 16 [get_pins u_clk/clk_64m]

