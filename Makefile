CC			=distcc g++ -std=c++17
CFLAGS		=-Ofast -msse2 -msse3 -mmmx -Ivendor/imgui/ -Ivendor/GLFW/include/ -Ivendor/glm/ -Ivendor/stb_image/ \
			-IWalnut/src/ -Icode/ -Ivendor/imgui/backends/ -Iinclude -Ivendor/
O			=obj
SDIR		=Walnut/src/Walnut
EXE			=Valden

ifndef release
CFLAGS     += -Og -g
else
CFLAGS 	   += -s
endif

OBJS=\
	$(O)/Image.o \
	$(O)/Random.o \
	$(O)/imgui_stdlib.o \
	$(O)/imgui.o \
	$(O)/imgui_draw.o \
	$(O)/imgui_tables.o \
	$(O)/imgui_widgets.o \
	$(O)/imgui_impl_sdl2.o \
	$(O)/imgui_impl_opengl3.o \
	$(O)/App/WalnutApp.o \
	$(O)/App/gui.o \
	$(O)/App/gln.o \
	$(O)/App/map.o \
	$(O)/App/preferences.o \
	$(O)/App/ImGuiFileDialog.o \
	$(O)/App/ImGuiTextEditor.o \
	$(O)/App/editor.o \
	$(O)/App/dialog.o \
	$(O)/App/stream.o \
	$(O)/App/shader.o \
	$(O)/App/command.o \
	$(O)/App/project.o \
	$(O)/App/ContentBrowserPanel.o \
	$(O)/App/shadermanager_dlg.o \
	$(O)/App/main.o \
	$(O)/App/events.o \
	$(O)/App/Application.o \
	$(O)/App/mapinfo_dlg.o \

.PHONY: all makedirs targets

MAKE=make
MKDIR=mkdir -p

targets: $(EXE)

makedirs:
	@if [ ! -d $(O)/ ];then $(MKDIR) $(O);fi
	@if [ ! -d $(O)/Input ];then $(MKDIR) $(O)/Input;fi
	@if [ ! -d $(O)/App ];then $(MKDIR) $(O)/App;fi
	@if [ ! -d $(O)/ImGuizmo ];then $(MKDIR) $(O)/ImGuizmo;fi

all: makedirs
	$(MAKE) targets

$(O)/%.o: vendor/imgui/misc/cpp/%.cpp
	$(CC) $(CFLAGS) -o $@ -c $<
$(O)/%.o: vendor/imgui/%.cpp
	$(CC) $(CFLAGS) -o $@ -c $<
$(O)/%.o: vendor/imgui/backends/%.cpp
	$(CC) $(CFLAGS) -o $@ -c $<
$(O)/%.o: $(SDIR)/%.cpp
	$(CC) $(CFLAGS) -o $@ -c $<
$(O)/Input/%.o: $(SDIR)/Input/%.cpp
	$(CC) $(CFLAGS) -o $@ -c $<
$(O)/App/%.o: code/%.cpp
	$(CC) $(CFLAGS) -o $@ -c $<
$(O)/ImGuizmo/%.o: vendor/ImGuizmo/%.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXE) -lglfw -lGL -lSDL2 -lbacktrace -lbz2 -lz

clean:
	rm $(OBJS)
