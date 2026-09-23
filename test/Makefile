# PC unit tests for the player logic. Run from this folder: make
# SRC can point at another copy of the sketch (e.g. an older commit).
SRC      ?= ../MP3Player
BUILD    ?= build
CXX      ?= c++
CXXFLAGS ?= -std=c++17 -g -Wall -Wextra
FLAGS     = $(CXXFLAGS) -Ishim -I$(SRC)

DEPS = $(SRC)/Player.cpp $(SRC)/Player.h $(SRC)/config.h shim/*.h shim/shim.cpp check.h

test: $(BUILD)/test_player_df $(BUILD)/test_player_sim
	$(BUILD)/test_player_df
	$(BUILD)/test_player_sim

$(BUILD)/test_player_df: test_player_df.cpp $(DEPS)
	@mkdir -p $(BUILD)
	$(CXX) $(FLAGS) -DUSE_DFPLAYER=1 -o $@ test_player_df.cpp shim/shim.cpp $(SRC)/Player.cpp

$(BUILD)/test_player_sim: test_player_sim.cpp $(DEPS)
	@mkdir -p $(BUILD)
	$(CXX) $(FLAGS) -DUSE_DFPLAYER=0 -o $@ test_player_sim.cpp shim/shim.cpp $(SRC)/Player.cpp

clean:
	rm -rf $(BUILD)

.PHONY: test clean
