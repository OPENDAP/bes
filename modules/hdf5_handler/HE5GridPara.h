    /// This file defines a few enum types used by HDF-EOS5 products
    // PixelRegistration
    enum EOS5GridPRType{
        HE5_HDFE_CENTER, HE5_HDFE_CORNER, HE5_HDFE_UNKNOWN, HE5_HDFE_MISSING
    };

    // GridOrigin
    enum EOS5GridOriginType{
        HE5_HDFE_GD_UL, HE5_HDFE_GD_UR, HE5_HDFE_GD_LL, HE5_HDFE_GD_LR,HE5_HDFE_GD_UNKNOWN, HE5_HDFE_GD_MISSING
    };

    // ProjectionCode
    enum EOS5GridPCType{
        HE5_GCTP_GEO,
        HE5_GCTP_UTM,
        HE5_GCTP_SPCS,
        HE5_GCTP_ALBERS,
        HE5_GCTP_LAMCC,
        HE5_GCTP_MERCAT,
        HE5_GCTP_PS,
        HE5_GCTP_POLYC,
        HE5_GCTP_EQUIDC,
        HE5_GCTP_TM,
        HE5_GCTP_STEREO,
        HE5_GCTP_LAMAZ,
        HE5_GCTP_AZMEQD,
        HE5_GCTP_GNOMON,
        HE5_GCTP_ORTHO,
        HE5_GCTP_GVNSP,
        HE5_GCTP_SNSOID,
        HE5_GCTP_EQRECT,
        HE5_GCTP_MILLER,
        HE5_GCTP_VGRINT,
        HE5_GCTP_HOM,
        HE5_GCTP_ROBIN,
        HE5_GCTP_SOM,
        HE5_GCTP_ALASKA,
        HE5_GCTP_GOOD,
        HE5_GCTP_MOLL,
        HE5_GCTP_IMOLL,
        HE5_GCTP_HAMMER,
        HE5_GCTP_WAGIV,
        HE5_GCTP_WAGVII,
        HE5_GCTP_OBLEQA,
        HE5_GCTP_CEA,
        HE5_GCTP_BCEA,
        HE5_GCTP_ISINUS,       
        HE5_GCTP_UNKNOWN,
        HE5_GCTP_MISSING
    };

