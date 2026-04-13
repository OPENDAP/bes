# Valgrind Output With Suppressed Errors

Example command uses `besstandalone -c tests/bes.conf -i tests/contiguous/coads_climatology.dmr` in `dmrpp-module`.

## 1. Generate Valgrind Output

Output file: `vg-dmrpp.txt`

```bash
cd ../hyrax/bes/modules/dmrpp-module
valgrind --leak-check=full --show-reachable=yes --error-limit=no \
  --gen-suppressions=all --log-file=vg-dmrpp.txt \
  besstandalone -c tests/bes.conf -i tests/contiguous/coads_climatology.dmr
```

If the log shows `Warning: client switching stacks?...`, use `--max-stackframe` to get clean output.

## 2. Generate Suppression File

Output file: `vg-dmrpp.supp`

```bash
cat vg-dmrpp.txt | /home/opendap/hyrax/bes/parse_valgrind_suppressions.sh > vg-dmrpp.supp
```

## 3. Run Valgrind With Suppressions

```bash
valgrind -s --leak-check=full --show-reachable=yes --error-limit=no \
  --suppressions=./vg-dmrpp.supp --gen-suppressions=all \
  --log-file=test_bes_make.log \
  besstandalone -c tests/bes.conf -i tests/contiguous/coads_climatology.dmr
```

You can edit the suppression file to remove repeated lines that match multiple similar errors.

## Reference

- https://wiki.wxwidgets.org/Valgrind_Suppression_File_Howto
