
PLATFORM := $(shell uname)

CPP_FLAGS = -c -Wall -pedantic

ifdef DEBUG
    CPP_FLAGS += -g3 -O0 -DDEBUG -D_DEBUG
    OBJDIR := $(PLATFORM)_objd
else
    CPP_FLAGS += -O3 -DNDEBUG -DRELEASE
    OBJDIR := $(PLATFORM)_objn
endif

ifeq ($(PLATFORM),Linux)
    CPP_FLAGS += -DLINUX -D_LINUX -D__LINUX__
endif

.DEFAULT : all

all : $(OBJDIR)/hd

.PHONY : clean

dep : $(DEP_FILES)

-include $(OBJ_FILES:.o=.d)

CPP_SRC_FILES := hd.cpp 

OBJ_LIST := $(CPP_SRC_FILES:.cpp=.o) $(C_SRC_FILES:.c=.o)
OBJ_FILES := $(addprefix $(OBJDIR)/, $(OBJ_LIST))
DEP_FILES := $(OBJ_FILES:.o=.d)

$(OBJDIR)/hd : $(OBJ_FILES) makefile
	@if [ ! -d $(@D) ] ; then mkdir -p $(@D) ; fi
	g++ -o $@ $(OBJ_FILES)

$(OBJDIR)/%.o : %.cpp makefile $(OBJDIR)/%.d
	@if [ ! -d $(@D) ] ; then mkdir -p $(@D) ; fi
	g++ $(CPP_FLAGS) -o $@ $<

$(OBJDIR)/%.d : %.cpp makefile
	@if [ ! -d $(@D) ] ; then mkdir -p $(@D) ; fi
	@echo "Generating dependencies for $<"
	@g++ $(CPP_FLAGS) -MM -MT $@ $< > $(@:.o=.d)

clean:
	rm -rf $(PLATFORM)_obj[dn]

