add_executable (sma_2d_test EXCLUDE_FROM_ALL sma_2d_test.cpp ../sma_2d.cpp)
add_executable(spectrum_metrics_test spectrum_metrics_test.cpp ../spectrum_metrics.cpp ../fourier.c)
add_test(NAME spectrum_metrics_test COMMAND spectrum_metrics_test)
add_executable(sweep_plan_test sweep_plan_test.cpp ../sweep_plan.cpp)
add_test(NAME sweep_plan_test COMMAND sweep_plan_test)
add_executable(loopback_reference_test loopback_reference_test.cpp
    ../loopback_reference.cpp ../sweep_plan.cpp)
add_test(NAME loopback_reference_test COMMAND loopback_reference_test)
