
CXX ?= g++
CFLAGS = -Wall -Wconversion -O3 -fPIC
SHVER = 2
OS = $(shell uname)
OBJECTS = list_page_classifier.o config.o utils.o SvmClassifier.o svm.o

body_extractor.o:
	$(CXX) $(CFLAGS) -g -c body_extractor.cpp dom_tree.cpp config.cpp utils.cpp boolean_classifier.cpp linear_classifier.cpp

list_page_classifier.o: DomTree.h config.h utils.h SvmClassifier.h
config.o: utils.h
utils.o: 
SvmClassifier.o: svm.h

svm.o:
	$(CXX) $(CFLAGS) -c svm.cpp
clean:
	rm -f *~ *.o test
