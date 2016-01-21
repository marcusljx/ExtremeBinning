all:run

EXECUTABLE=ExtremeBinning
libs=-lssl -lcrypto
flags=$(libs)

SAMPLE_FILE=sampleFile.txt
INPUTS=$(SAMPLE_FILE)

# This program uses a copy of itself (main.cpp) as the sample test file
run: $(EXECUTABLE).bin
	# cp main.cpp sampleFile.txt
	./$(EXECUTABLE).bin $(INPUTS)

$(EXECUTABLE).bin: main.cpp
	g++ main.cpp -o $(EXECUTABLE).bin $(flags)

%.o: %.cpp %.h
	g++ -c $< $(flags)

clean:
	rm *.bin *.o