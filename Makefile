#
# MIT licence
#

PROJDIR := ${CURDIR}

target := libbutter.so
exe_target := butter
SRCS = $(wildcard src/*.c)
TST_SRCS += $(wildcard tst/*.c)
OBJS := $(SRCS:%.c=%.o)
TST_OBJS := $(TST_SRCS:%.c=%.o)
GCNO := $(SRCS:%.c=%.gcno)
TST_GCNO := $(TST_SRCS:%.c=%.gcno)
GCDA := $(SRCS:%.c=%.gcda)
TST_GCDA := $(TST_SRCS:%.c=%.gcda)
#CFLAGS := -Wall -Wextra -Werror -fPIC -std=gnu11
#CFLAGS := -Wall -Wextra -Werror -fPIC
CFLAGS := -Wall -Werror -fPIC -std=gnu11

EXE_SRCS = $(SRCS)
EXE_SRCS += main.c
EXE_OBJS := $(EXE_SRCS:%.c=%.o)

ifdef TMPDATAPATH
DATAPATHOPT := -DTMPDATAPATH=$(TMPDATAPATH)
else
DATAPATHOPT := 
endif

ifdef debug_mode
PREFIX ?= "${HOME}/local-gcc-memcheck/"
CFLAGS += -g3 -fsanitize=address -fno-omit-frame-pointer -DSTATIC=
LDFLAGS := -L${PREFIX}/lib
LDFLAGS += -lasan -fsanitize=address
CFLAGS += -fprofile-arcs -ftest-coverage
LDFLAGS += -fprofile-arcs -ftest-coverage -lgcov
else
PREFIX ?= ${HOME}/local
CFLAGS += -O2 -DSTATIC=static
LDFLAGS := -L${PREFIX}/lib
endif

ifdef debug_mode
CFLAGS += -DDEBUG_MODE=1
endif

LDFLAGS += -lpthread -lssl -lcrypto

#GCC ?= clang
GCC ?= gcc

CCINCLUDES := -I$(PROJDIR)

%.o:%.c
	@$(GCC) $(CFLAGS) $(CCINCLUDES) $(DATAPATHOPT) -c $< -o $@

all: tags __depend__ $(target) $(exe_target)

__depend__: $(SRCS)
	@$(GCC) $(CFLAGS) $(CCINCLUDES) -MM $^ > $@

-include __depend__

test-case-list.h: ${TST_SRCS}
	@tst/create-tc-list.sh

$(target): $(OBJS)
	@${GCC} -shared -Wl,-E -fPIC $^ -o $@ $(LDFLAGS)

clean: force
	@rm -f $(OBJS) $(TST_OBJS) $(EXE_OBJS)
	@rm -f $(GCNO) $(TST_GCNO) $(GCDA) $(TST_GCDA)
	@rm -f *.gcno *.gcda
	@rm -fr coverage coverage.info.txt
	@rm -f __depend__ tags
	@rm -f test-case-list.h
	@rm -f $(target) $(exe_target) a.out

install: $(target) $(exe_target)
	mkdir -p $(PREFIX)/lib/
	mkdir -p $(PREFIX)/bin/
	mkdir -p $(PREFIX)/include/
	cp -f $(target) $(PREFIX)/lib/
	cp -f $(exe_target) $(PREFIX)/bin/
	cp -f public.h $(PREFIX)/include/butter.h

uninstall: force
	rm -f $(PREFIX)/lib/$(target)
	rm -f $(PREFIX)/bin/$(exe_target)
	rm -f $(PREFIX)/include/butter.h

test: a.out

a.out: test-case-list.h $(EXE_OBJS) $(TST_OBJS)
	@${GCC} $^ -o $@ $(LDFLAGS)

butter: $(EXE_OBJS)
	@${GCC} $^ -o $@ $(LDFLAGS)

tags: force
ifeq ($(shell uname), Linux)
	-@ctags -R *
endif

force:
