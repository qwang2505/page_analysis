GTEST = ../../gtest-1.6.0/make/gtest_main.a
PARAMS = ../../gtest-1.6.0/make/gtest_main.a -I.. -I../../gtest-1.6.0/include/ -lpthread

all: utils_test SvmClassifier_test config_test list_page_classifier_test

utils_test: utils_test.cpp $(GTEST)
	g++ utils_test.cpp ../utils.cpp -o utils_test $(PARAMS)

config_test: config_test.cpp $(GTEST)
	g++ -g config_test.cpp ../config.cpp ../utils.cpp -o config_test $(PARAMS)

list_page_classifier_test: list_page_classifier_test.cpp $(GTEST)
	g++ -g list_page_classifier_test.cpp ../list_page_classifier.cpp ../config.cpp ../utils.cpp ../SvmClassifier.cpp ../svm.o -o list_page_classifier_test $(PARAMS)

boolean_classifier_test: boolean_classifier_test.cpp ../boolean_classifier.h ../utils.h $(GTEST)
	g++ -g boolean_classifier_test.cpp ../boolean_classifier.cpp ../utils.cpp -o boolean_classifier_test $(PARAMS)

body_extractor_test: body_extractor_test.cpp
	g++ -g body_extractor_test.cpp ../body_extractor.o ../dom_tree.o ../config.o ../utils.o ../boolean_classifier.o ../linear_classifier.o -o body_extractor_test -lpython2.6 $(PARAMS)
  
SvmClassifier_test: SvmClassifier_test.cpp $(GTEST)
	g++ SvmClassifier_test.cpp ../SvmClassifier.cpp ../svm.o -o SvmClassifier_test $(PARAMS)


