
BUILD_FLAGS = -L$(prefix)/deps/lib -lhdf5 -I$(prefix)/deps/include
progs = h5_obj_addr_info obtain_vlstr_addr obtain_vlstr_addr_hard_code_subset read_str_vars test_read_vl_offset

all: prefix-set
	for d in $(progs); do $(MAKE) $(MFLAGS) $$d; done

.PHONY: prefix-set
prefix-set:
	@if test -z "$$prefix"; then \
	echo "The env variable 'prefix' must be set. See README"; exit 1; fi

h5_obj_addr_info: h5_obj_addr_info.cc
	g++  $(BUILD_FLAGS) -o h5_obj_addr_info h5_obj_addr_info.cc

obtain_vlstr_addr: obtain_vlstr_addr.cc
	g++  $(BUILD_FLAGS) -o obtain_vlstr_addr obtain_vlstr_addr.cc

obtain_vlstr_addr_hard_code_subset: obtain_vlstr_addr_hard_code_subset.cc
	g++  $(BUILD_FLAGS) -o obtain_vlstr_addr_hard_code_subset obtain_vlstr_addr_hard_code_subset.cc

read_str_vars: read_str_vars.cc
	g++  $(BUILD_FLAGS) -o read_str_vars read_str_vars.cc

test_read_vl_offset: test_read_vl_offset.cc
	g++  $(BUILD_FLAGS) -o test_read_vl_offset test_read_vl_offset.cc

clean:
	rm $(progs)

install:
	cp ${progs} ${prefix}/bin

uninstall:
	cd ${prefix}/bin/ && rm -vf ${progs}
