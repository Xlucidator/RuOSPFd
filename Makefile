include config.mk

.PHONY: all clean

all : 
	cd src/ && $(MAKE)
	cp src/$(TARGET) ./

clean:
	rm $(TARGET)
	cd src/ && $(MAKE) clean
