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
LDFLAGS += -lasan -fsanitize=address
CFLAGS += -g3 -fsanitize=address -fno-omit-frame-pointer -DSTATIC=
else
ifdef test_mode
LDFLAGS += -lasan -fsanitize=address
LDFLAGS += -fprofile-arcs -ftest-coverage -lgcov
CFLAGS += -g3 -fsanitize=address -fno-omit-frame-pointer -DSTATIC=
CFLAGS += -fprofile-arcs -ftest-coverage
CFLAGS += -DTEST_MODE=1
else
CFLAGS += -O2 -DSTATIC=static
endif
endif

LDFLAGS += -lpthread -lssl -lcrypto

GCC ?= clang

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
