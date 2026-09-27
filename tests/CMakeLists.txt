add_executable (sma_2d_test EXCLUDE_FROM_ALL sma_2d_test.cpp ../sma_2d.cpp)
add_executable(spectrum_metrics_test spectrum_metrics_test.cpp ../spectrum_metrics.cpp ../fourier.c)
add_test(NAME spectrum_metrics_test COMMAND spectrum_metrics_test)
