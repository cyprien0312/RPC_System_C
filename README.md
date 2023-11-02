#  Remote Procedure Call

 
## 1. Project Overview

Remote Procedure Call (RPC) is a crucial technology in distributed computing that enables software applications to communicate with each other seamlessly over a network. It provides a way for a client to call a function on a remote server as if it were a local function call. This abstraction allows developers to build distributed systems and applications that span multiple machines and platforms.

In this project, you will be building a custom RPC system that allows computations to be split seamlessly between multiple computers. This system may differ from standard RPC systems, but the underlying principles of RPC will still apply.

Your RPC system must be written in C. Submissions that do not compile and run on a Linux cloud VM, like the one you have been provided with, may receive zero marks. You must write your own RPC code, without using existing RPC libraries.

## 2. RPC System Architecture

Your task is to design and code a simple Remote Procedure Call (RPC) system using a client-server architecture. The RPC system will be implemented in two files, called `rpc.c` and `rpc.h`. The resulting system can be linked to either a client or a server. For marking, we will write our own clients and servers, and so you must stick to the proposed API carefully.

For testing purposes, you may run server and client programs on the same machine (e.g., your VM).

## 3. Project Details

Your task is to design and code the RPC system described above. You will design the application layer protocol to use. A skeleton is provided which uses a simple application programming interface (API). When we assess your submission, we will link our own testing code using the same RPC system API; what you will be assessed on is `rpc.c` (and any other supporting files compiled in by your Makefile).

Note that implementing the API will require you to use sockets. This uses material covered in the lectures after the project is released.
