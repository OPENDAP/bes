#!/bin/sh
#
# This script downloads NASA test data from DAACs and MediaFire.
#
# Check whether "wget" or "curl" is available first.
GET=""
command -v  wget > /dev/null && GET="wget -N --retr-symlinks" 
if [ -z "$GET" ]; then
  command -v  curl > /dev/null && GET="curl -O -C -"
fi

if [ -z "$GET" ]; then
  echo "Neither wget nor curl found in your system."
  exit
fi

# GESDISC 
## OMI
### Swath

#### Use new version later.
#### https://data.gesdisc.earthdata.nasa.gov/data/Aura_OMI_Level2/OMBRO.003/2012/339/OMI-Aura_L2-OMBRO_2012m1204t1200-o44623_v003-2014m0413t034956.he5
$GET https://download1582.mediafire.com/bct274ovwdhgU3ZHWfn5GOj51aropKZwKZn504yCVoprxeGDfgafpnZpCEw7e4n945Kqtj-Pv1xSkgu6zPZhIFo2oQe0eFF8OPHta8JrkA2g3VujiLf_jVsJ6e5P_m0eUvSuEHJu4WVOFMIt1Y8yZdHs11FwNB2no5tTbn5rQkhVHD0/79e99u2qv55kda5/OMI-Aura_L2-OMBRO_2012m1204t1200-o44623_v003-2012m1204t185453.he5

#### https://data.gesdisc.earthdata.nasa.gov/data/Aura_OMI_Level2/OMNO2.003/2016/046/OMI-Aura_L2-OMNO2_2016m0215t0210-o61626_v003-2019m0819t155523.he5
$GET https://download1590.mediafire.com/kejzulbu2dzg-ZE2zocYU11zNT-24rAIZ38UV5t33WhRGefC_XsszHzAgTojRPgu_DIyWI7mjAf9rNHoIL1fCWvGpPvyOdgRYZE0a8-rzZ7wE8odSbemXdfVV9lJXS7w1mRAKnjSD3WBIfXBT0uORAWQ7G_SpbAxwoeqhBx9Re_NWEw/ja73hcn1iz1g90p/OMI-Aura_L2-OMNO2_2016m0215t0210-o61626_v003-2016m0215t200753.he5

##### https://data.gesdisc.earthdata.nasa.gov/data/Aura_OMI_Level2/OMUVB.003/2006/004/OMI-Aura_L2-OMUVB_2006m0104t0019-o07831_v003-2013m0910t114559.he5
$GET https://download856.mediafire.com/os6c1h2xqvhgxCQzc9hMekKN8ou9xRBD5zR-vZtrm3cjb-LwmDdXspwrmg3mhZ8xT-600IJvQR-bkOQ8KcNo1f_A7EiZEBnJBPUGoM60ip15O3mhSq-RvmUJBmEvtdCoM7EyW-zBBpcTXH6_2Rox2MO9lDiN3kG__oE7cQsCLkrFZiA/zdg1ryao3gujbup/OMI-Aura_L2-OMUVB_2006m0104t0019-o07831_v003-2008m0716t0249.he5

##### https://data.gesdisc.earthdata.nasa.gov/data/Aura_OMI_Level2G/OMTO3G.003/2004/OMI-Aura_L2G-OMTO3G_2004m1001_v003-2012m0404t145232.he5
$GET https://download1326.mediafire.com/v90jq5flyuwgb7or4I1uBNrU7J_e2v8h9waEMMh1Ept5Cqtw1kBzOSNDCNhUxHe1JQd1YQyluBNmPNuwINAyBd_MYp1VA4ovcc173QeUjjgiawn7xvtK8BHRJohOHpzuSkCheWnoqJTKcJ8B57GRi3Xtl2KawMviL-T0GxzErTq824Y/2cbcsf7avniup8f/OMI-Aura_L2G-OMTO3G_2004m1001_v003-2009m0602t123920.he5


### Grid
#### https://data.gesdisc.earthdata.nasa.gov/data/Aura_OMI_Level3/OMAERUVd.003/2009/OMI-Aura_L3-OMAERUVd_2009m0109_v003-2017m0821t135414.he5
$GET https://download1076.mediafire.com/1qkdaipgkq2g3940TcEnNJtwIlcyOjOFmKHIdSjWeQfclgP0HSQJVFAYSDgf2lM6Zmd1t1NCLuAfNHs540sdoBcyiUKreSLBSHYx3tGqxom6dH8Ud2Pf3mA_9TXfK0k5_E5KRA0Rj1n9N9WmLsbPIliuaudL3bOTXCPwOpBPXl0PBxw/r8nyun11yf0s80x/OMI-Aura_L3-OMAERUVd_2009m0109_v003-2011m1203t141123.he5

##### https://data.gesdisc.earthdata.nasa.gov/data/Aura_OMI_Level3/OMSO2e.003/2012/OMI-Aura_L3-OMSO2e_2012m0101_v003-2020m1015t191334.he5
$GET https://download1076.mediafire.com/a9k9ifbdahmgiZ4HWAdfA0Qb-L5W5XElRIA53A9aAoYIqrN48h91blwD7dOdbresZ52LDRutLMumX6k4jKF6NjUwznBn1sXBXYyOIjeGqeuQNEfQ8AWYQE63_dvlIfTM9gmDq6Sdf1aL5yk-A88XhJN5hXoSWjjNQtbWGpJruhv41ww/owpwfohhzirsde5/OMI-Aura_L3-OMSO2e_2012m0101_v003-2012m0103t014459.he5

##### https://data.gesdisc.earthdata.nasa.gov/data/Aura_OMI_Level3/OMAEROe.003/2012/OMI-Aura_L3-OMAEROe_2012m0213_v003-2012m0531t045139.he5
$GET https://download1590.mediafire.com/eofqgq2pbqgggdEVuMn67LF3CFzyYVUoavohAbIaiX509PUGqD4mnxd5hNKk9t-ehbdhNJuCXxe7A7uHMQWG7It6-jBBcaGDRx1PPnk5HoQYq-UGTr5ZgRT-Z5iHbu5NevY7r7-W8_6vFCb5w8g9ec0PpJBj8yLP1O20gQmGGaUt8NE/at67hugccfpfoon/OMI-Aura_L3-OMAEROe_2012m0213_v003-2012m0215t021315.he5

$GET https://download1586.mediafire.com/v1dbqq3gtxtgypjf4dQlybAaFC8WwhaN98vPgiMPJH7EPbZOl-Tmzgax4KADovjvCpIpPKj22gKPRa3WISAjKJJA8hZZLyMz1c5S8MHHaZKmMXNRrVDlVya_PYyrO47ozvRmmRoO2sz4XD0L_jd4IJ2UiLQE-0bcB3bczAtFMELWbMs/x1ccaci4n6qe8vr/OMI_L3.nc4.h5

## MLS
### Single swath
##### https://data.gesdisc.earthdata.nasa.gov/data/Aura_MLS_Level2/ML2BRO.005/2010/MLS-Aura_L2GP-BrO_v05-01-c01_2010d255.he5
$GET https://download1076.mediafire.com/vyyjoew1rl2gf1EOTzWMJM61BOTSEVolgnFes10V2YZtb5eoga-cHDtU_Zit6tkE9IrIudMPhZgGX-CR1m--jRTeuIhwbNMQB7h5r43fHtGUDhfd9Hs9-T00bztu33WOfiG2njW8J4C17gDkY9MeFnEoEGmwSU3fMSu35ditMna5InA/q026rpr25cm5y5h/MLS-Aura_L2GP-BrO_v02-23-c01_2010d255.he5

### Single swath - Near Real Time product
##### MLS2O3_NRT; Since it is a NRT product, there's no 2013 file any more.
$GET https://download1474.mediafire.com/r33q65i62regRKxYhVA4i-caB2-OpExMPCXgO1TTnQ3nvFLgwVlfZzcoeeupucprmpsDIkVyIma2QPa1a5OUNAsp_WkCZ3livlh5eHkXLPi636gR61B_qmtrKFM2L9we9_tu8r5oxfWbacuRUID7DUJHFCmk9oH5Y6OPfPB1sp2YGiQ/pdwrzey0xt12blu/MLS-Aura_L2GP-O3_v03-40-NRT-06-c01_2013d024t0010.he5

### Multiple swaths 
#####  ML2T; https://data.gesdisc.earthdata.nasa.gov/data/Aura_MLS_Level2/ML2T.005/2011/MLS-Aura_L2GP-Temperature_v05-01-c01_2011d316.he5
$GET https://download1349.mediafire.com/zv1zgdc49dsgxEVpv7LzwdvyIGcEcqEYLN4jxV8o0efeIsSju6TaipfbQx-cMi1TUp7VqpifTT1tDdvRlOUyk_mHwtvhuyVeXWtGzCiNE_DlcwsGkIqNpspXX5CRV7yd81mhrY5qp3DaDK3r_vzWsOJk5_zHkYZCojnCM4pM_9wmN4M/diq3m977r7di7ha/MLS-Aura_L2GP-Temperature_v03-33-c01_2011d316.he5


## HIRDLS
### Swath
#### HIRDLS2;  https://data.gesdisc.earthdata.nasa.gov/data/Aura_HIRDLS_Level2/HIRDLS2.007/2008/HIRDLS-Aura_L2_v07-00-20-c01_2008d001.he5
$GET https://download1580.mediafire.com/315gj2dq3ymg1tNOWqM4Dpmlr-jZZd1jDB28IrPyxE3EoKM5_v-tuLiAkEECutivL8ichmqGiAmRvUUB0kC61uBaqm77d4ixtypLzpDLpi_wRQbvIHkQGpMsTeSQKRZlm1l-DaWlf8RfZwmFksm8iwS1ge3t5IZwZ1KdHAb9Aept/dbbu2aqz26meh4x/HIRDLS-Aura_L2_v06-00-00-c01_2008d001.he5

### ZA
#### n/a
$GET https://download1589.mediafire.com/c4ffx7wo4n8g8WdZ8eKMLMuZDTFmXVZln5q3tEyNa2TsT94UAf7-hBfcqy6GsC2eMgTgfPf7eO8dBUAi-wjA7_XhCgLVpU06kzxQiDdNOaSaWD-ABz5Dv0OJJVwl3ESU3y9V7KpUUYkbzv88CUrYxGB4innuJJVd3r0G93HgU5ID/ede4eu5l4thxj0p/HIRDLS-Aura_L3ZAD_v06-00-00-c02_2005d022-2008d077.he5

### Level 3 Stratospheric Column
#### HIR3SCOL; https://data.gesdisc.earthdata.nasa.gov/data/Aura_HIRDLS_Level3/HIR3SCOL.007/HIRDLS-Aura_L3SCOL_v07-00-20-c01_2005d022-2008d077.he5
$GET https://download1320.mediafire.com/r4omo3htjxug4UjZonGi253lYHjDOmHkubGhY71EJV-41r0Sq0D7GT9TUhYRJuTszQOaYY9MNzYQ00ypOE0NB0s4NKUrkxSMuHoD6hrVggLTgi5jc8bRK_D-XU0NF5GvfcXjR0-yuMC7q_TNL6BWgi87EDlKu-12NzFiKT-cfAZ9/a5an76pjimmouaq/HIRDLS-Aura_L3SCOL_v06-00-00-c02_2005d022-2008d077.he5


## (S)BUV (MEaSUREs Ozone)
### Swath
#### SBUV2N17L2; https://data.gesdisc.earthdata.nasa.gov/data/Ozone/SBUV2N17L2.1/2011/SBUV2-NOAA17_L2-SBUV2N17L2_2011m1231_v01-02-2013m0828t143157.h5
$GET https://download1592.mediafire.com/9dffbwvk52zgKiO51HLQY9j_69HFiKRzzzgpvFm3VaNpT_56OBVKAWKjY5eoqxN1EOelWNVtLL00jXgPkBtKc5rrq99slxrK7TkMeNEZqolIU3JSLhORW_igdkHSLBtrRIUHAEeiNZB2Y-RYr-FblsSLB3YcuY3FD-BTtl7cQ5dG/gui7pbo865nw4o6/SBUV2-NOAA17_L2-SBUV2N17L2_2011m1231_v01-01-2012m0905t152911.h5

### ZA
#### BUVN04L3zm; https://data.gesdisc.earthdata.nasa.gov/data/Ozone/BUVN04L3zm.1/BUV-Nimbus04_L3zm_v01-02-2013m0422t101810.h5
$GET https://download1325.mediafire.com/guwsl6kr5elgwisDjN-LtuZU7OyT7j94x3LkP_JmbBcwjCnVWXbzj_hOJcfV6_X7B6Iyuw44wHhhonFiNRvdVYe4ggbYdsfzT8CrJND4tbPZIdNB1FcaH0qjiXS0W6CS02r0_N2EXayjO-Cz0cT7miVUiSwCCT6claKi0rO8npyw/h495ivggtf2yzip/BUV-Nimbus04_L3zm_v01-00-2012m0203t144121.h5


## SWDB (MEaSUREs)
### SWDB_L310
#### https://data.gesdisc.earthdata.nasa.gov/data/DeepBlueSeaWiFS_Level3/SWDB_L310.004/2010/DeepBlue-SeaWiFS-1.0_L3_20100101_v004-20130604T131317Z.h5
$GET https://download1588.mediafire.com/hqyts7xwr6xgU3AD1fQ2aUWn0LMmuTXrlJJj1JJTKuatIrdi9NhnCruqwsD4VblfE-gRXEkyz8ao7QQE4i_ZzFqSCTkOoJIXNxWmXRStwa83zLDtb4FcwnxmY2P4K_Wyaw4bqMtN2amIrEN_r026Co-C2D3xcc4DXxYLWhsO4PRK/11kcszsapi2y02i/DeepBlue-SeaWiFS-1.0_L3_20100101_v002-20110527T191319Z.h5

#### No 19970903 granule
#### https://data.gesdisc.earthdata.nasa.gov/data/DeepBlueSeaWiFS_Level3/SWDB_L310.004/1997/DeepBlue-SeaWiFS-1.0_L3_19970904_v004-20130603T205559Z.h5
$GET https://download1591.mediafire.com/xvgiuwyh9rgg8qRILflUVsjWxwqSWn8ZbAcQlX47uZBIXUpNrbJJAkWXk9WN3YZd_7dwyUy0rVJp7gqNJQbYp6VHBp9eueVwwvDbzN1_bRgCkMypxaMOmIUbcDFUfoJSeg6sPrAcbKzRrLl6dGeGyUzR03jFAQPcs36IGOl7ynCQ/6h78k58qeimy8bo/DeepBlue-SeaWiFS-1.0_L3_19970903_v003-20111127T185012Z.h5

## GSSTF (MEaSUREs)
### GSSTF
####  https://data.gesdisc.earthdata.nasa.gov/data/GSSTF/GSSTF.2c/2008/GSSTF.2c.2008.01.01.he5
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/GSSTF.2b.2008.01.01.he5
### GSSTFYC
$GET https://data.gesdisc.earthdata.nasa.gov/data/GSSTF/GSSTFYC.3/GSSTFYC.3.Year.1988_2008.he5


## GOZCARD (MEaSUREs)
### GozSmlpHCl
#### $GET https://data.gesdisc.earthdata.nasa.gov/data/GOZCARDS/GozSmlpHCl.1/GOZ-Source-MLP_HCl_ev1-01_2005.nc4
$GET https://download1323.mediafire.com/2z6wzozvztwg5lL7H-sqzeLGyhNhuL9on0VVvPswBR-Fjm4gql6PnspWm4pT1TAdEz-mUzdJ47T8didWY47xWTtQ3oS9EevfL2M_MZBaFNKdH29EqZBHIkIU17Qg490rdhX5yqeVlSXP-dI1ec9zAmGSZOu_g_tTw0LVDVcvcSjB/b9ybp8pnh9mx271/GOZ-Source-MLP_HCl_ev1-00_2005.nc4.h5


## AIRS(M)_CPR_MAT - Multi-Sensor Water Vapor with Cloud Climatology (MEaSUREs)
### AIRS_CPR_MAT
#### https://data.gesdisc.earthdata.nasa.gov/data/AIRS_CloudSat/AIRS_CPR_MAT.3.2/2006/166/matched-airs.aqua_cloudsat-v3.2-2006.06.15.239_airs.nc
$GET https://download1527.mediafire.com/nqaff7dmw9kg0C-c2gEPCzmXizi_TOJTWhc-X6lmRxDDW2MzfpQ2SPhpOesWKEE8pQAb2JwBLLmvW9vI0ORJLbH1XC-w52mj2kIXckPR5kFlfZ_VIpS_SrOWPZpKXRJSgpTmQTSaJJ0lcL327Qc8spbCiFQ1sgpJR68tA9kXieG6/u0iibs1pdfln6jv/matched-airs.aqua_cloudsat-v3.1-2006.06.15.239_airs.nc4.h5

### AIRSM_CPR_MAT
#### https://data.gesdisc.earthdata.nasa.gov/data/AIRS_CloudSat/AIRSM_CPR_MAT.3.2/2011/070/matched-airs.aqua_cloudsat-v3.2-2011.03.11.001_amsu.nc
$GET https://download1478.mediafire.com/l3opjcvs6iog-ziO1-rIppYg_HuIDK4swfsO5F2dm5sW-ykVDekj10LAFe7YM0xMzieWCiJkR3qau1Y01oAfqarthkVWRN_Dyz--J1KWOmfLqlVm44MgaZR98UruuwyY5McNCQ4n_VZ8uhEspxkvUDvZRW-PZreZMl0qZP3v6MLA/ij8mop1g3weo018/matched-airs.aqua_cloudsat-v3.1-2011.03.11.001_amsu.nc4.h5

## GOSAT/acos
### ACOS_L2S
#### https://data.gesdisc.earthdata.nasa.gov/data/GOSAT_TANSO_Level2/ACOS_L2S.9r/2011/109/acos_L2s_110419_43_B9200_PolB_190721012155.h5
$GET https://download1323.mediafire.com/enjlysfeg4rgfVaSh9xVTBYmDjIcxIx2-VRglkDEvfamKXRo9aloyNmlxANDGfBiDRS3tAIVqIqdYTtr-zW--uXbFFSG9gT5gIcS_Eqt8JXC6oRlEbJ7RfTKxBYpXcf7c_GSic6hw2YZXZmlAE0AXfGdEI9o9wukp7w8MGVmexXS/9gu4kq6b1zht3hn/acos_L2s_110419_43_Production_v110110_L2s2800_r01_PolB_110430192739.h5

## OCO2
### OCO2_L2_Standard
#### https://data.gesdisc.earthdata.nasa.gov/data/OCO2_DATA/OCO2_L2_Standard.10r/2015/089/oco2_L2StdND_03945a_150330_B10004r_191126232527.h5
$GET https://download1640.mediafire.com/bv391yzaprjggp1FAhvd-zye6GlOEg5JHm0bruKV6YchOK6xf0myH5CIz_KMroqnklY6z7sU6lUKBKdpUui_0dlXD-dYuIrgHwGb0_tY-U5W-gsvvv2_dHuFGt7Qu_LF1hYLNwvJ0xByl40iJq5J4IwoAtS3FJlucjh2DtAZyvj3/jakbsg12y2lvlyu/oco2_L2StdND_03945a_150330_B6000_150331024816.h5

### OCO2_L2_Lite_FP
#### https://data.gesdisc.earthdata.nasa.gov/data/OCO2_DATA/OCO2_L2_Lite_FP.11.1r/2018/oco2_LtCO2_180102_B11100Ar_230602001852s.nc4
$GET https://download1349.mediafire.com/ovr5lxqwy7pg2mcL-0Qf4E5z0utY6E9wBvnkrFRo3AY8fGsXLu1MMwyqWwixEYPh83C8qDJzROZYiiNliFWCrWlbGsPX3wzA-FIPxuLSdtgQrfjL8mn6ApH66gESuZayFpbZRr4Y05k9j5woDBwlIdFp0EqIOxlrhfUTrlRQtPPI/a2hd64rfavvwur1/oco2_LtCO2_180102_B9003r_180929105747s.nc4.h5


# GSFC
## mabel (ICESat-2)
$GET https://download1503.mediafire.com/zdnnly1pk0wg6sV51XUyW9SbB2ymx7wE6_TMT_fkmY1tLV-MEt4X8-ZQuq1oUIwwIaMydBnvz9HgKecet9oE9VVtp3iYa9Rmehua03pctDSulbusFLjw7kX4doNhRm86_kNKRZo9bVOBbq4os3ahgzNkQemgbA6b5acKyeps3vJ-/1ycgjf7svnv50td/mabel_l2a_20110322T165030_005_1.h5


# LaRC ASDC
## TES
### TL2O3N
#### https://l5ftl01.larc.nasa.gov/ops/tesl1l2l3/TES/TL2O3N.008/2009.12.20/TES-Aura_L2-O3-Nadir_r0000011015_F08_12.he5
$GET https://download1325.mediafire.com/hwtfy83d006gTVebYo7tKvuQefm27jZCmhz0nL4bgUL6yxnYWSYEjaJBAAlqSnkL4t6FryoZ7A7jdnp3WGx3sU8JxCvWIlHsVI2qQJw6MqutMxg2o8nPrbGX6RF_bU4GM3GxJJxXrjLm_M5BSJCeldJrQyg6qR2r3RaxNLsSmw_H/ilox3qqdhh7cqxy/TES-Aura_L2-O3-Nadir_r0000011015_F05_07.he5


### TL3CH4D
##### https://l5ftl01.larc.nasa.gov/ops/tesl1l2l3/TES/TL3CH4D.006/2009.03.31/TES-Aura_L3-CH4_r0000010410_F01_12.he5
$GET https://download1592.mediafire.com/bkbqmmx0fejg49X--b9OV28XQ9_xMl6OUhXC3cIfYvmcHOwZgvcCzmhzdKUUScS_0wpDnHiv8Hs2lF8mIH-XArqXrtmcAtq1QK5a3z-moOVVGAKIDecQP6mzTINk7kh-M3ckNTuk1xpXqJZ33_2uzDDRr11cWT9LCv562U3OwIpB/j1hv83wqhrlrbux/TES-Aura_L3-CH4_r0000010410_F01_07.he5


# Hybrid EOS5 file
## AirMSPI_ACEPOL_Ellipsoid-projected_Georegistered_Radiance_Data
### NASA distributes this product in HDF4.
$GET https://download1527.mediafire.com/e66irtnt7vkgBF5tQYdaWHiL57hZawcU6czYLpx-JI73AwlROvldxWKV0cxro_JCmM7R1-hKof18uHfh7Va7Q_Vb5Pt1QH6L4Adn6CErW26iC1fYaPq84YRPhXULbMRfSpA_vQzMweYyraoeaQCpPD34-08xUKLi7fILEbqMhg-M/vfclybruno3h4aw/AirMSPI_ER2_GRP_ELLIPSOID_20161006_181726Z_CA-NewberrySprings_SWPF_F01_V006.he5

# NSIDC
## GLAS
### GLAH13_034
#### https://data.nsidc.earthdatacloud.nasa.gov/nsidc-cumulus-prod-protected/GLAS/GLAH13/034/2009/10/GLAH13_634_2131_001_1317_0_01_0001.H5
$GET https://download848.mediafire.com/zlnb6a1108ogtwInv8Pp7stHBILCikcImsNBkg3Ns_U1CqmvuygUxKlXZJmdtvwaLg8jsL6mxauRpEgqkJHNprO7CFfE_ZXOaKAolpbZZ3D7U_Lj_qRHor6l8It1VFzWiVkdN88xI_gtdqjCDQT-M2gYz0GoTwt9n_2w7uXjHQThrdM/atom741qfn76kh5/GLAH13_633_2103_001_1317_0_01_0001.h5

# PO.DAAC
## Aquarius
### AQUARIUS_L2_SSS_V5; 2011237 is the oldest.
#### https://archive.podaac.earthdata.nasa.gov/podaac-ops-cumulus-protected/AQUARIUS_L2_SSS_V5/Q2011237000100.L2_SCI_V5.0
$GET https://download937.mediafire.com/lpj7mukww7tg_Lop1fcMy9PXs-1b4PJ7sOoEgdGSDpHmDFDTwrDvDGWf-6O7p2c9ToUHDXYIxB-C_ObGBSZQ4nZaYxGKzttt0JrwMqHm9CvAY33r6pNQ6JtsAzTHGHMKL1E9PuDUCORd2-mE-fhEm_qurd3NKmLyQHUVE8FRKo2ts-4/amcncegida89yt4/Q2011149002900.L2_SCI_V1.0.bz2.0.bz2.0.h5

### n/a
$GET https://download944.mediafire.com/32fn5s08f2agSI0KJJur7kzOqzA_wZ5cSPu-AiOYJwJHHC2IADmrD3QI-FUdzJdFHfKngs1rlQcOLXxoANiPBfuM9zpMcIcekI5gBZZw3jUy_9QyaGeLlRFBba5Wzlbmy-2eCpZnGR8gy9Ne9KPUBYWIcBcn_WrWdbKUSWpXqVYF/bxc1qnop1okoay3/Q20111722011263.L3b_SNSU_EVSCI_V1.2.main.h5

### n/a
$GET https://download948.mediafire.com/r08d5kb8cfhgW5y4DPIcFhzsycnHH9ZzGDUsAwvksnirpCfYnKmhfPQqrgLSPzMoJhAK6NlmOzMxfJhmRyhRcHJBloEmCARDpAnYjfe071Wig2j2W2IkZ5zT1mV2MZoVZVW4RPO8OAiS9Kx-BrKjaUPiPVND3qet-s_ntWz3hKTg/b8gk65o5en22wju/Q20103372010343.L3m_7D_SCIB1_V1.0_SSS_1deg.h5

### n/a
$GET https://download1530.mediafire.com/w948nbka4iqg9mq66s1-1Te_syWvfEFD0GSAyGoE00ADaDW5m2_c9p-rY-5aomn-J15oyEjGWMMsVZFq82tZRiCln4B8uXWfJZvuJtk6wpyY0ohgjbzIWhEPs-5r0yzz9VX2vUbl8nbVF5mvXf1LvgaJpY1qYqMLtSfLWLaAVPVk/h7o35nas2c0fp76/Q2012034.L3m_DAY_SCI_V5.0_SSS_1deg.h5


# GESDISC GPM
## GPM files.
### GPM_1AGMI
#### https://data.gesdisc.earthdata.nasa.gov/data/GPM_L1A/GPM_1AGMI.07/2014/064/1A.GPM.GMI.COUNT2021.20140305-S061920-E075148.000087.V07A.HDF5
$GET https://download1655.mediafire.com/vnh4kvidf08gBfsW0vaoKXqFAVyzH6y8Wau3v09nQFSAkoUwHzTbYvl8rd8axoQfrrwioP7szwxPXKBh_bWsQT9biagydDYDzQSYPNU9I6y75lrYUIYl7PrcWURKfFFXC18lBX1sfcp2nuBn69nKAar57YAXUpTOGPLIw7TJLyYc/7pwkvrlqk53jzzt/1A.GPM.GMI.COUNT2014v3.20140305-S061920-E075148.000087.V03A.h5

### GPM_2AGPROFGPMGMI
#### https://data.gesdisc.earthdata.nasa.gov/data/GPM_L2/GPM_2AGPROFGPMGMI.07/2014/264/2A.GPM.GMI.GPROF2021v1.20140921-S002001-E015234.003195.V07A.HDF5
$GET https://download1339.mediafire.com/ia0gixsedi1gmUNT8B7OQnPmaRd-xHfOuyFlA-ocqAoF8SyFLAAG8GD_C7ciVOx5-dW_QAnoFJENZNKzuNf6rUWSdAXHRpCALMbqWvDd22EZ6j70AY53C2HdXJgt_6adHJg9R96fYH_ybis4aWW3NNgQG4tei17SHk5zK49UnIW0/t8fb5itmdp6puzs/2A.GPM.GMI.GPROF2014v1-4.20140921-S002001-E015234.003195.V03C.h5

### GPM_3GPROFGPMGMI
#### https://data.gesdisc.earthdata.nasa.gov/data/GPM_L3/GPM_3GPROFGPMGMI.07/2024/3A-MO.GPM.GMI.GRID2021R1.20240601-S000000-E235959.06.V07D.HDF5
$GET https://download1509.mediafire.com/g3f6ntt55lxgYQ8Ycznfbg1GsCcdb7KfqsqbhFR0Sf18SjqJHwFwBPTex_tGCn50wqIXMW0Q59JJY28Iu7bzZzYEnXClYjWsMnrMaTHAZ5V16P_cw6yH7RmUvn_XFeEgK-FMRNw7U6fugaJa-eRzCPb4aANa4sAR15CayPHaRxBT/hjxq1564bllej3u/3A-MO.GPM.GMI.GRID2014R1.20140601-S000000-E235959.06.V03A.h5

 
# HFVHANDLER-129
## n/a
$GET https://download1586.mediafire.com/r0hj7k98ip1gm0n70M8S6fUd351yC8Vv_yF48PksgiO9GwNk-HCFTEJOAgXTpJ_a-cDEIUgdzB6GyIJMDRG0Z1Z-CNHkSWMc0dqcHKkyraCq-ZPPM7YTipfR7p3KBo1EzSX8ig5lHddEczDEPDGrAI9zcRohyg4sKfRP1MFs8e8e/ass0nlqvsryfiqn/good_imerge.h5

## LPRM_AMSR2_DS_SOILM2
$GET https://data.gesdisc.earthdata.nasa.gov/data/WAOB/LPRM_AMSR2_DS_SOILM2.001/2012/184/LPRM-AMSR2_L2_DS_D_SOILM2_V001_20120702231838.nc4
mv LPRM-AMSR2_L2_DS_D_SOILM2_V001_20120702231838.nc4 LPRM-AMSR2_L2_D_SOILM2_V001_20120702231838.nc4.h5

## LPRM_AMSR2_A_SOILM3
$GET https://data.gesdisc.earthdata.nasa.gov/data/WAOB/LPRM_AMSR2_A_SOILM3.001/2012/12/LPRM-AMSR2_L3_A_SOILM3_V001_20121216010911.nc4

## n/a
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/NASA1/OBPG/A20030602003090.L3m_MO_AT108_CHL_chlor_a_4km.h5

## SeaWiFS_L3m_CHL
### https://oceandata.sci.gsfc.nasa.gov/cmr/getfile/S20030602003090.L3m_MO_CHL_chlor_a_9km.nc
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/NASA1/OBPG/S20030602003090.L3m_MO_ST92_CHL_chlor_a_9km.h5

## MOP03T
### https://l5ftl01.larc.nasa.gov/ops/misrl2l3/MOPITT/MOP03T.009/2013.11.29/MOP03T-20131129-L3V5.9.1.he5
$GET https://download850.mediafire.com/8pfvdz0fpvegxC7wXmuPM9KQjfXiUZ30dXYGaDELfITGVkKVnHcKhdQ3MeodKdJCu6iSNj3-0KZzVTEtnC74RtTsu0IXIsQ0pvQ0TtJ6daI4st6JE2K5-jD7E6UO8vf_osJkjJMwgteYRM0vQcBuhAt8YSQpSvY9JdJS_ZLVjxWw/1gvmdxcybxqmuiu/MOP03T-20131129-L3V4.2.1.h5

# 2-D lat/lon netCDF-4 like file
## TOMSN7AER
### https://data.gesdisc.earthdata.nasa.gov/data/AER/TOMSN7AER.2/1991/181/TOMS-N7_L2-TOMSN7AER_1991m0630t084305-o64032_v02-00-2018m0904t083738.h5
$GET https://download1334.mediafire.com/u3mdb3qukl0gAuwURd6ImKyjxuKZHDYUZMYWcJ01jxtsfKsdauXW0_OPHLQ6Dki_iVgqi28l1v_hgWWoGgD-LDz7IQGeVmZEhVCdx7hbeo4eRO4Ncr_ARYcLucKsPhSnvplrskxT0_boinBhOZdART4APMJpPWz9rUXM4_c942ja/pzb8j7k3mq44fzn/TOMS-N7_L2-TOMSN7AERUV_1991m0630t0915-o64032_v02-00-2015m0918t123456.h5

## n/a
$GET https://download943.mediafire.com/51xef4ax76ogi2dsN7e26Kj_ya8GkNFzSlT-Cf8D33qXJ7hsCpGlRo0BqOhR_v1v25wpuqK2yWXoIy9GNQ3KqXE9kwaPVy95C2UQdgMXYd7aU0OUDqpHwMP-fxZGWNLY4RJ8IPai5GKYyzYY9PsEmfcett6cWMcLKHajKVYhGyt_/rv2pvrh279nzca9/S5PNRTIL2NO220180422T00470920180422T005209027060100110820180422T022729.nc.h5

## OMIAuraAER
$GET https://data.gesdisc.earthdata.nasa.gov/data/AER/OMIAuraAER.1/2006/227/OMI-Aura_L2-OMIAuraAER_2006m0815t130241-o11086_v01-00-2018m0911t083242.h5


# SWDB files for testing the memory cache
## SWDB_L305
$GET https://data.gesdisc.earthdata.nasa.gov/data/DeepBlueSeaWiFS_Level3/SWDB_L305.004/2010/DeepBlue-SeaWiFS-0.5_L3_20100613_v004-20130604T133539Z.h5

$GET https://data.gesdisc.earthdata.nasa.gov/data/DeepBlueSeaWiFS_Level3/SWDB_L305.004/2010/DeepBlue-SeaWiFS-0.5_L3_20101210_v004-20130604T135845Z.h5

## SWDB_L310
$GET https://data.gesdisc.earthdata.nasa.gov/data/DeepBlueSeaWiFS_Level3/SWDB_L310.004/2010/DeepBlue-SeaWiFS-1.0_L3_20100613_v004-20130604T133539Z.h5

$GET https://data.gesdisc.earthdata.nasa.gov/data/DeepBlueSeaWiFS_Level3/SWDB_L310.004/2010/DeepBlue-SeaWiFS-1.0_L3_20100614_v004-20130604T133548Z.h5

## SWDB_L2
$GET https://data.gesdisc.earthdata.nasa.gov/data/DeepBlueSeaWiFS_Level2/SWDB_L2.004/2010/001/DeepBlue-SeaWiFS_L2_20100101T003505Z_v004-20130524T141300Z.h5


# LPDAAC sinusodial projections one grid and multiple grid
## VNP09A1
$GET https://e4ftl01.cr.usgs.gov//DP101/VIIRS/VNP09A1.001/2015.09.14/VNP09A1.A2015257.h29v11.001.2017122154929.h5
## VNP09GA
$GET https://e4ftl01.cr.usgs.gov//DP103/VIIRS/VNP09GA.001/2017.06.10/VNP09GA.A2017161.h10v04.001.2017162212935.h5

# GHRC PS and LAMAZ projections
## n/a
$GET https://download1532.mediafire.com/abmcmvwrvqmgZ_nUyxaxA5PikVQg1Jb3RCLUOK_ifIxTacLGklAaBUg1Ik4y-CSyIMNNJZHM2lIQ1CYOaYqwse0fDC1BCDYO3BlQo6gle415XFJFfs2SOAaEVSha3fei7weJUmhlVLiCNUJZRgD2X5RzoMGufgGaHMzler_7QyvW/7la2uqacxsi3epq/AMSR_2_L3_DailySnow_P00_20160831.he5

## n/a
$GET https://download1592.mediafire.com/ytivwl7fcvyg-KYfnOouTW5XAbNez7MUaXtia12sLVKWxzHuC0eaGwCAc7auOYiPJEKOETDaGF5ajCjIhf9tG-33h9Fa6u8R1Fa1mSgkFAcdrMb0uPWCkW3dVUtarOZJ7wtIO4S1Bf1oA0FM9rBkIMXDyCzc8gM491R4LhPnzgoU/9lgqb70tijv35gn/AMSR_2_L3_SeaIce12km_P00_20160831.he5


# OMPS-NPP level 3 daily(long variable names)
## OMPS_NPP_NMTO3_L3_DAILY
$GET https://data.gesdisc.earthdata.nasa.gov/data/SNPP_OMPS_Level3/OMPS_NPP_NMTO3_L3_DAILY.2/2018/OMPS-NPP_NMTO3-L3-DAILY_v2.1_2018m0102_2018m0104t012837.h5

# Arctas-Car
## n/a
$GET https://download1336.mediafire.com/dwpmzt8z58ugARqz--1GQnI0pAoaQ11fpCZjoxgDWhxjtGC5hW6mTlT2rfFI5JtdpGLeVej8e1I2zBPHjuKNT9JAm-XPGRKWaBLUCAz9WfFGm24LqP0VrtfmGHrM5oPJaPYMOgK6rbSnsAOv77UJnl1rRy39S_1u-v5mP0-1nuPI/yxsklewso2jq8v0/Arctas-car_p3b_20080407_2002_Level1C_20171121.nc.h5

# SMAP level 3
## SPL3SMP
### https://data.nsidc.earthdatacloud.nasa.gov/nsidc-cumulus-prod-protected/SMAP/SPL3SMP/009/2015/04/SMAP_L3_SM_P_20150406_R19240_001.h5
$GET https://download1591.mediafire.com/2k2zxdaclv5gGGcG8aDDBozNYVMcpD6Fr9Ec9tIWl7PqoKDTj6E-4i03fRnTO69CTwp4vA_GAQzrPAOQTzjFlOaIWInsyhWs-F0L1OgdeE0ZV8696V3vTGsHBSZ8Kq2IR9sFzx91c4hqcymJmpFc2kSLc1lDQ7ni5hc_Ksr4VpUP/mvukdauq4az7ijt/SMAP_L3_SM_P_20150406_R14010_001.h5

# GPM level 3 DPR
## n/a
$GET https://download1500.mediafire.com/nx4unowmvclg64NIuuX2hyMpPMQl9yGl_HluQfITwMfhN2F03yoe0XCrxUvpwaGOq1rM3Mx4KpdWxC9NAGS4QK683NFBSP3ZrhhAUfyaEnsWk2hLkpzokfzZ9ccb0LcbDaBq3P_ppmFXydKCTQlDAnjgcu9un8PNqrUYtfpo_pp9/szi8fhmacqho9lt/3A.GPM.DPR.algName.20180331-S221135-E234357.076185.V00B.HDF5

# PODAAC GHRSST
## MUR-JPL-L4-GLOB-v4.1
$GET https://archive.podaac.earthdata.nasa.gov/podaac-ops-cumulus-protected/MUR-JPL-L4-GLOB-v4.1/20020602090000-JPL-L4_GHRSST-SSTfnd-MUR-GLOB-v02.0-fv04.1.nc
mv 20020602090000-JPL-L4_GHRSST-SSTfnd-MUR-GLOB-v02.0-fv04.1.nc 20020602090000-JPL-L4_GHRSST-SSTfnd-MUR-GLOB-v02.0-fv04.1.h5

# daymet
## Daymet_Daily_V4R1_2129
$GET https://data.ornldaac.earthdata.nasa.gov/protected/daymet/Daymet_Daily_V4R1/data/daymet_v4_daily_na_prcp_2010.nc
mv daymet_v4_daily_na_prcp_2010.nc daymet_v4_daily_na_prcp_2010.nc.h5

# f16ssmi
## n/a
$GET https://download1500.mediafire.com/m4ap64ot951grPz5YL8CA50ZdSbbwBPnCacB2lKrxAvNc2kOrsnGkR2bnsdZbobjkhGWnm1vpYPp2527CtORB3r6P2nNa_3AMOUkut6eEXaWw35Y7sYMqfom2dwydaqx_eVtKALVZUC-Iz6RR96lb4xBGH_F0cbmJykfZdR8jz2Q/gyiiaagfolum3f1/f16_ssmis_20031026v7.nc.h5

# SMAP_L3_SM_P_E
## SPL3SMP_E
### https://n5eil01u.ecs.nsidc.org/DP4/SMAP/SPL3SMP_E.006/2021.10.30/SMAP_L3_SM_P_E_20211030_R19240_001.h5
$GET https://download1503.mediafire.com/r0s20hh79lxgICvvYJ3_TgqNUs9ykWJLzcUQpSb9SYiSlIFsbXtrxdiEivrkf0h1mCS7UnLPSXdMTgauWFKi5o0RMRYPhKgSucrH94UBnuTqgaG8fJOUR15-xk2sqZNutDiLSWbXdYBFjPvbMOLBn-4AVTeFzLHJBcXuaqsrU8Gd/axi0emmjiioe70w/SMAP_L3_SM_P_E_20211030_R18240_001.h5
