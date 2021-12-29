
AUTOMAKE_OPTIONS = foreign 

# Build the bes.conf used for testing so that the value substituted for
# @abs_top_srcdir@ does not contain '../'. This happens when using
# configure's value for the parameter when running the distcheck target.
%.conf : %.conf.in
	@clean_abs_top_srcdir=`python -c "import os.path; print(os.path.abspath('${abs_top_srcdir}'))"`; \
	sed -e "s%[@]abs_top_srcdir[@]%$$clean_abs_top_srcdir%" \
		-e "s%[@]abs_top_builddir[@]%${abs_top_builddir}%" $< > $@

build_responses.conf: build_responses.conf.in $(top_srcdir)/configure.ac


fnoc.dap: fnoc1.dap.bescmd
	besstandalone -c build_responses.conf -i $< > $@