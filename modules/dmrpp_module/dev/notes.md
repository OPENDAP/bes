

## Africa_Congo_Jason3_13.nc
_Contains 2 Scalar Strings._

    ID(Type is String)
    sat(Type is String)

### Test app outcomes
#### h5_obj_addr_info
    % h5_obj_addr_info Africa_Congo_Jason3_13.nc sat
    Storage: contiguous
    Addr: 63480
    Size: 6
    % h5_obj_addr_info Africa_Congo_Jason3_13.nc ID
    Storage: contiguous
    Addr: 63457
    Size: 15



## Asia_Yenisei1kmdaily.nc
_Contains one (small) 1D String Array_

    sat[ /charlength= 0..21] (Type is String)
### Test app outcomes
#### h5_obj_addr_info
    % h5_obj_addr_info Asia_Yenisei1kmdaily.nc /sat
    Storage: contiguous
    Addr: 75389
    Size: 208120

## NOAA_PRAWLER_L2_20100905_20200106_VER001.nc
_Contains 1 large 1D String Array_

    Epoch_Time[ /obs= 0..183814] (Type is String)

### Test app outcomes
#### h5_obj_addr_info
    % h5_obj_addr_info NOAA_PRAWLER_L2_20100905_20200106_VER001.nc Epoch_Time
    storage: chunked.
    Number of dimensions in a chunk is  2
    Chunk Dim 0 is 183815
    Chunk Dim 1 is 8
    total_num_chunks is 1
    Number of chunks is 1
    Chunk index:  0
    Number of bytes:  323386
    Logical offset: offset[0][0]
    Physical offset: 756683



## SWOT_L2_HR_PIXCVec_006_509_207L_20220801T115408_20220801T115418_Dx0000_01.nc
_Contains 5 larger 1D String Array_

    reach_id[ /points= 0..614181] (Type is String)
    node_id[ /points= 0..614181] (Type is String)
    lake_id[ /points= 0..614181] (Type is String)
    obs_id[ /points= 0..614181] (Type is String)
    ice_clim_f[ /points= 0..614181] (Type is Int16)

### Test app outcomes
#### h5_obj_addr_info
    % h5_obj_addr_info SWOT_L2_HR_PIXCVec_006_509_207L_20220801T115408_20220801T115418_Dx0000_01.nc reach_id
    storage: chunked.
    Number of dimensions in a chunk is  2
    Chunk Dim 0 is 307091
    Chunk Dim 1 is 6
    total_num_chunks is 4
    Number of chunks is 4
    Chunk index:  0
    Number of bytes:  28928
    Logical offset: offset[0][0]
    Physical offset: 4473132
    
        Chunk index:  1
        Number of bytes:  29245
        Logical offset: offset[0][6]
          Physical offset: 4443887
    
        Chunk index:  2
        Number of bytes:  14856
        Logical offset: offset[307091][0]
          Physical offset: 4502060
    
        Chunk index:  3
        Number of bytes:  14499
        Logical offset: offset[307091][6]
          Physical offset: 4516916

    % h5_obj_addr_info SWOT_L2_HR_PIXCVec_006_509_207L_20220801T115408_20220801T115418_Dx0000_01.nc node_id
    storage: chunked.
    Number of dimensions in a chunk is  2
    Chunk Dim 0 is 307091
    Chunk Dim 1 is 7
    total_num_chunks is 4
    Number of chunks is 4
    Chunk index:  0
    Number of bytes:  31266
    Logical offset: offset[0][0]
    Physical offset: 4534031
    
        Chunk index:  1
        Number of bytes:  39236
        Logical offset: offset[0][7]
          Physical offset: 4565297
    
        Chunk index:  2
        Number of bytes:  16505
        Logical offset: offset[307091][0]
          Physical offset: 4604533
    
        Chunk index:  3
        Number of bytes:  17878
        Logical offset: offset[307091][7]
          Physical offset: 4625134

    % h5_obj_addr_info SWOT_L2_HR_PIXCVec_006_509_207L_20220801T115408_20220801T115418_Dx0000_01.nc lake_id
    storage: chunked.
    Number of dimensions in a chunk is  2
    Chunk Dim 0 is 307091
    Chunk Dim 1 is 5
    total_num_chunks is 4
    Number of chunks is 4
    Chunk index:  0
    Number of bytes:  27623
    Logical offset: offset[0][0]
    Physical offset: 4643012
    
        Chunk index:  1
        Number of bytes:  28375
        Logical offset: offset[0][5]
          Physical offset: 4670635
    
        Chunk index:  2
        Number of bytes:  28277
        Logical offset: offset[307091][0]
          Physical offset: 4701058
    
        Chunk index:  3
        Number of bytes:  28437
        Logical offset: offset[307091][5]
          Physical offset: 4729335

    % h5_obj_addr_info SWOT_L2_HR_PIXCVec_006_509_207L_20220801T115408_20220801T115418_Dx0000_01.nc obs_id
    storage: chunked.
    Number of dimensions in a chunk is  2
    Chunk Dim 0 is 307091
    Chunk Dim 1 is 7
    total_num_chunks is 4
    Number of chunks is 4
    Chunk index:  0
    Number of bytes:  66530
    Logical offset: offset[0][0]
    Physical offset: 4760388
    
        Chunk index:  1
        Number of bytes:  82611
        Logical offset: offset[0][7]
          Physical offset: 4826918
    
        Chunk index:  2
        Number of bytes:  73089
        Logical offset: offset[307091][0]
          Physical offset: 4909529
    
        Chunk index:  3
        Number of bytes:  87107
        Logical offset: offset[307091][7]
          Physical offset: 4982618

    %h5_obj_addr_info SWOT_L2_HR_PIXCVec_006_509_207L_20220801T115408_20220801T115418_Dx0000_01.nc ice_clim_f
    storage: chunked.
    Number of dimensions in a chunk is  1
    Chunk Dim 0 is 614182
    total_num_chunks is 1
    Number of chunks is 1
    Chunk index:  0
    Number of bytes:  3163
    Logical offset: offset[0]
    Physical offset: 5075917


## SWOT files with one 1D String Array
### SWOT_L2_LR_SSH_Expert_001_001_20111113T000000_20111113T005105_DG10_01.nc
    polarization_karin[ /num_lines= 0..9867] (Type is String)

#### Test app outcomes
##### h5_obj_addr_info
    % h5_obj_addr_info SWOT_L2_LR_SSH_Expert_001_001_20111113T000000_20111113T005105_DG10_01.nc polarization_karin
    storage: chunked.
    Number of dimensions in a chunk is  2
    Chunk Dim 0 is 9868
    Chunk Dim 1 is 2
    total_num_chunks is 1
    Number of chunks is 1
    Chunk index:  0
    Number of bytes:  43
    Logical offset: offset[0][0]
    Physical offset: 2642254


### SWOT_L2_LR_SSH_Expert_001_001_20111113T000000_20111113T005126_DG10_01.nc
    polarization_karin[ /num_lines= 0..9865] (Type is String)

#### Test app outcomes
##### h5_obj_addr_info
    %h5_obj_addr_info SWOT_L2_LR_SSH_Expert_001_001_20111113T000000_20111113T005126_DG10_01.nc polarization_karin
    storage: chunked.
    Number of dimensions in a chunk is  2
    Chunk Dim 0 is 9866
    Chunk Dim 1 is 2
    total_num_chunks is 1
    Number of chunks is 1
    Chunk index:  0
    Number of bytes:  43
    Logical offset: offset[0][0]
    Physical offset: 2656596

### SWOT_L2_LR_SSH_Expert_001_001_20140412T120000_20140412T125105_DG10_01.nc
    polarization_karin[ /num_lines= 0..9867] (Type is String)

#### Test app outcomes
##### h5_obj_addr_info
    % h5_obj_addr_info SWOT_L2_LR_SSH_Expert_001_001_20140412T120000_20140412T125105_DG10_01.nc polarization_karin
    storage: chunked.
    Number of dimensions in a chunk is  2
    Chunk Dim 0 is 9868
    Chunk Dim 1 is 2
    total_num_chunks is 1
    Number of chunks is 1
    Chunk index:  0
    Number of bytes:  43
    Logical offset: offset[0][0]
    Physical offset: 3007094

### SWOT_L2_LR_SSH_Expert_001_001_20140412T120000_20140412T125126_DG10_01.nc
    polarization_karin[ /num_lines= 0..9865] (Type is String)
#### Test app outcomes
##### h5_obj_addr_info
    % h5_obj_addr_info SWOT_L2_LR_SSH_Expert_001_001_20140412T120000_20140412T125126_DG10_01.nc polarization_karin
    storage: chunked.
    Number of dimensions in a chunk is  2
    Chunk Dim 0 is 9866
    Chunk Dim 1 is 2
    total_num_chunks is 1
    Number of chunks is 1
    Chunk index:  0
    Number of bytes:  43
    Logical offset: offset[0][0]
    Physical offset: 3011281



## rutgers-ru26d-L2-20190905T2055.nc
_Contains three 2D Arrays of String_

    segment[ /time= 0..3675] [ /profile_id= 0..1] (Type is String)
    the8x3_filename[ /time= 0..3675] [ /profile_id= 0..1] (Type is String)
    profile_dir[ /time= 0..3675] [ /profile_id= 0..1] (Type is String)

### Test app outcomes
#### h5_obj_addr_info
    
    % h5_obj_addr_info rutgers-ru26d-L2-20190905T2055.nc segment
    storage: chunked.
    Number of dimensions in a chunk is  3
    Chunk Dim 0 is 3676
    Chunk Dim 1 is 2
    Chunk Dim 2 is 18
    total_num_chunks is 1
    Number of chunks is 1
    Chunk index:  0
    Number of bytes:  1005
    Logical offset: offset[0][0][0]
    Physical offset: 390383
    
    % h5_obj_addr_info rutgers-ru26d-L2-20190905T2055.nc the8x3_filename
    storage: chunked.
    Number of dimensions in a chunk is  3
    Chunk Dim 0 is 3676
    Chunk Dim 1 is 2
    Chunk Dim 2 is 8
    total_num_chunks is 1
    Number of chunks is 1
    Chunk index:  0
    Number of bytes:  397
    Logical offset: offset[0][0][0]
    Physical offset: 396572
    
    % h5_obj_addr_info rutgers-ru26d-L2-20190905T2055.nc profile_dir
    storage: chunked.
    Number of dimensions in a chunk is  3
    Chunk Dim 0 is 3676
    Chunk Dim 1 is 2
    Chunk Dim 2 is 1
    total_num_chunks is 1
    Number of chunks is 1
    Chunk index:  0
    Number of bytes:  60
    Logical offset: offset[0][0][0]
    Physical offset: 489174
    




