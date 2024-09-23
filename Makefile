cc := clang++
cflags := -std=c++20 -g -Wall -Wextra -Werror -I. -ftemplate-backtrace-limit=0
server_t := server
server_s := ipc_cpp/server.cpp ipc_cpp/mem.cpp
server_o := $(patsubst %.cpp,%.o,$(server_s))
server_d := $(patsubst %.cpp,%.d,$(server_s))
server_c := $(cflags)
server_l := -lboost_system -lboost_thread libraylib.a
driver_t := driver
driver_s := driver.cpp $(server_s) program.cpp program_gfx/program_gfx.cpp
driver_o := $(patsubst %.cpp,%.o,$(driver_s))
driver_d := $(patsubst %.cpp,%.d,$(driver_s))
driver_c := $(cflags)
driver_l := -lboost_system -lboost_thread libraylib.a
driver_gfx_t := driver_gfx
driver_gfx_s := driver_gfx.cpp $(server_s) program.cpp program_gfx/program_gfx.cpp
driver_gfx_o := $(patsubst %.cpp,%.o,$(driver_gfx_s))
driver_gfx_d := $(patsubst %.cpp,%.d,$(driver_gfx_s))
driver_gfx_c := $(cflags)
driver_gfx_l := libraylib.a -lm -lboost_system -lboost_thread libglm.a
test_allocator_t := test_allocator
test_allocator_s := test_allocator.cpp $(server_s)
test_allocator_o := $(patsubst %.cpp,%.o,$(test_allocator_s))
test_allocator_d := $(patsubst %.cpp,%.d,$(test_allocator_s))
test_allocator_c := $(cflags)
test_allocator_l := 
test_asio_t := test_asio
test_asio_s := test_asio.cpp $(server_s)
test_asio_o := $(patsubst %.cpp,%.o,$(test_asio_s))
test_asio_d := $(patsubst %.cpp,%.d,$(test_asio_s))
test_asio_c := $(cflags)
test_asio_l := -lboost_system -lboost_thread
test_mutex_t := test_mutex
test_mutex_s := test_mutex.cpp $(server_s)
test_mutex_o := $(patsubst %.cpp,%.o,$(test_mutex_s))
test_mutex_d := $(patsubst %.cpp,%.d,$(test_mutex_s))
test_mutex_c := $(cflags)
test_mutex_l := -lboost_system -lboost_thread

.phony: all
all: $(driver_t)
all: $(driver_gfx_t)
all: $(test_allocator_t)
all: $(test_asio_t)

%.o: %.cpp
	$(cc) -c $< -o $@ $(cflags) -MMD -MP -MF $(<:.cpp=.d)

$(server_t): $(server_o)
	$(cc) $(server_o) -o $@ $(server_l)

$(driver_t): $(driver_o) $(server_t)
	$(cc) $(driver_o) -o $@ $(driver_l)

$(driver_gfx_t): $(driver_gfx_o) $(server_t)
	$(cc) $(driver_gfx_o) -o $@ $(driver_gfx_l)

$(test_allocator_t): $(test_allocator_o) $(server_t)
	$(cc) $(test_allocator_o) -o $@ $(test_allocator_l)

$(test_asio_t): $(test_asio_o) $(server_t)
	$(cc) $(test_asio_o) -o $@ $(test_asio_l)

$(test_mutex_t): $(test_mutex_o) $(server_t)
	$(cc) $(test_mutex_o) -o $@ $(test_mutex_l)

.phony: clean
clean:
	- rm $(server_d) $(server_o) $(server_t) \
		$(driver_d) $(driver_o) $(driver_t) \
		$(driver_gfx_d) $(driver_gfx_o) $(driver_gfx_t) \
		$(test_allocator_d) $(test_allocator_o) $(test_allocator_t) \
		$(test_asio_d) $(test_asio_o) $(test_asio_t) \
		$(test_mutex_d) $(test_mutex_o) $(test_mutex_t)

.phony: re
re: clean
re: all

-include $(driver_d) $(driver_gfx_d) $(test_allocator_d) $(test_asio_d)
