cc := clang++
cflags := -std=c++20 -g -Wall -Wextra -Werror -I. -ftemplate-backtrace-limit=0
driver_t := driver
driver_s := driver.cpp ipc_cpp/mem.cpp program.cpp program_gfx/program_gfx.cpp
driver_o := $(patsubst %.cpp,%.o,$(driver_s))
driver_d := $(patsubst %.cpp,%.d,$(driver_s))
driver_c := $(cflags)
driver_l := -lboost_system -lboost_thread libraylib.a
driver_gfx_t := driver_gfx
driver_gfx_s := driver_gfx.cpp ipc_cpp/mem.cpp program.cpp program_gfx/program_gfx.cpp
driver_gfx_o := $(patsubst %.cpp,%.o,$(driver_gfx_s))
driver_gfx_d := $(patsubst %.cpp,%.d,$(driver_gfx_s))
driver_gfx_c := $(cflags)
driver_gfx_l := libraylib.a -lm -lboost_system -lboost_thread libglm.a
test_allocator_t := test_allocator
test_allocator_s := test_allocator.cpp ipc_cpp/mem.cpp
test_allocator_o := $(patsubst %.cpp,%.o,$(test_allocator_s))
test_allocator_d := $(patsubst %.cpp,%.d,$(test_allocator_s))
test_allocator_c := $(cflags)
test_allocator_l := 
test_asio_t := test_asio
test_asio_s := test_asio.cpp ipc_cpp/mem.cpp
test_asio_o := $(patsubst %.cpp,%.o,$(test_asio_s))
test_asio_d := $(patsubst %.cpp,%.d,$(test_asio_s))
test_asio_c := $(cflags)
test_asio_l := -lboost_system -lboost_thread

.phony: all
all: $(driver_t)
all: $(driver_gfx_t)
all: $(test_allocator_t)
all: $(test_asio_t)

%.o: %.cpp
	$(cc) -c $< -o $@ $(cflags) -MMD -MP -MF $(<:.cpp=.d)

$(driver_t): $(driver_o)
	$(cc) $^ -o $@ $(driver_l)

$(driver_gfx_t): $(driver_gfx_o)
	$(cc) $^ -o $@ $(driver_gfx_l)

$(test_allocator_t): $(test_allocator_o)
	$(cc) $^ -o $@ $(test_allocator_l)

$(test_asio_t): $(test_asio_o)
	$(cc) $^ -o $@ $(test_asio_l)

.phony: clean
clean:
	- rm $(driver_d) $(driver_o) $(driver_t) \
		$(driver_gfx_d) $(driver_gfx_o) $(driver_gfx_t) \
		$(test_allocator_d) $(test_allocator_o) $(test_allocator_t) \
		$(test_asio_d) $(test_asio_o) $(test_asio_t)

.phony: re
re: clean
re: all

-include $(driver_d) $(driver_gfx_d) $(test_allocator_d) $(test_asio_d)
