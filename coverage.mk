
# Build the code for test coverage analysis
# jhrg 11/17/20

.PHONY: coverage

if ENABLE_COVERAGE
AM_CXXFLAGS += --coverage -pg
AM_LDFLAGS += --coverage -pg

coverage: 
	-gcovr --sonarqube -r . $(GCOVR_FLAGS) > gcovr_report.txt

endif
