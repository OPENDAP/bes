Attributes {
    AttrContainer {
        Int32 Attr_1 0, 1, 2, 3, 4, 5, 6, 7, 8, 9;
        String Description "Test for whether a container gets merged correctly.";
    }
    NC_GLOBAL {
        String Dataset_Description "Virtual union aggregation example";
        String Description "Dataset One";
    }
    SelfReferentialVariable {
    }
    Foo {
        String Description "This var names Foo comes first and should be in output.";
    }
    Bar {
        String Description "This var names Bar comes first and should be in output.";
    }
    Baz {
        String Description "This var names Baz comes first and should be in output.";
    }
}
