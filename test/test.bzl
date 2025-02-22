
def lightning_unit_test(src):
    native.cc_test(
        name = src.split('.')[0],
        size = "small",
        srcs = [src],
        copts = ["-std=c++17"],
        deps = [
            "//:lightning",
            "@googletest//:gtest",
            "@googletest//:gtest_main",
        ],
    )

def create_unit_tests(srcs):
    for src in srcs:
        lightning_unit_test(src=src)