Attributes {
    NC_GLOBAL {
        String Wrappee_Description "This is the wrapped NcML file for the reentrancy test.";
        String WrapperDescription "The combined data set will include this one as well as the WrappeeDescription!";
    }
    Wrapped_Wrappee_Variable {
        String Description "This should be modified from the wrappee in a successful wrap!";
    }
}
