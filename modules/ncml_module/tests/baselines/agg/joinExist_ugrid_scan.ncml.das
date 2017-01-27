Attributes {
    fvcom_mesh {
        String face_node_connectivity "fnca";
        String standard_name "mesh_topology";
        Int32 topology_dimension 2;
        String node_coordinates "X Y";
    }
    X {
        String grid "element";
        String grid_location "node";
    }
    Y {
        String grid "element";
        String grid_location "node";
    }
    fnca {
        Int32 start_index 1;
        String standard_name "face_node_connectivity";
    }
    time {
    }
    twoDnodedata {
        String coordinates "Y X";
        String mesh "fvcom_mesh";
        String location "node";
    }
}
