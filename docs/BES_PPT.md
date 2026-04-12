# OPeNDAP PPT/TfTP Programmer's Manual

National Center for Atmospheric Research (NCAR)  
High Altitude Observatory (HAO)  
Boulder, CO.

This manual will guide you through using the OPeNDAP PPT/TfTP library. It contains information about the different macros needed to communicate between a standalone client and server.

Patrick West  
`pwest@hao.ucar.edu`

## Overview

The OPeNDAP back-end server uses a communication protocol called PPT (Point to Point Transport), or TfTP, involving handshaking between the client and server. The protocol strings are defined in the class `PPTProtocol`. Each string is defined within the scope of this class. For example, to use the `PPTCLIENT_TESTING_CONNECTION` string you would say:

```
PPTProtocol::PPTCLIENT_TESTING_CONNECTION
```

## 1. Making a connection to the server

When a client wishes to communicate with a server it first sends the message: `PPTCLIENT_TESTING_CONNECTION`.

Upon receiving this initialization request the server responds with the message `PPTSERVER_CONNECTION_OK`. If there is a problem with the connection then the server responds with `PPT_PROTOCOL_UNDEFINED`.

## 2. Sending messages to and receiving responses from the server

Once a connection has been established with a server the client now sends requests to and receives responses from the server.

Each request that the client sends to the server and each response sent from the server to the client must end with the string `PPT_COMPLETE_DATA_TRANSMITION`. This tells the receiving end that the request or response is complete and the server or client can handle it.

The `PPT_COMPLETE_DATA_TRANSMITION` string, either client or server, can be a separate buffer sent from the client to the server (or vice versa) or can be the last string in a buffer sent between the two. In other words, a client could send two buffers, one with the request string in it and the second with the `PPT_COMPLETE_DATA_TRANSMITION` string in it. Or, the string can be a part of one buffer sent.

## 3. Ending a connection

When the client is ready to end the connection or the server is exiting, it sends the message `PPT_EXIT_NOW` to the other. The other end should immediately catch this message and clean up their respective connections.
