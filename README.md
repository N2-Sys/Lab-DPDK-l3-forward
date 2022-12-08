# Lab-DPDK

## Environment Setup

> Note: The version of DPDK is 21.11 LTS

In order to be able to simulate the environment of multiple different machines, we need to create two different virtual machines using the Virtual Machine tool.
We will set up N+1 NICs on the first virtual machine to simulate N different endpoint machines and N+1 NICs on the second machine to simulate an N port L3 router.
The extra port is used to test the program for interaction.

For each terminal machine and the network port on the second machine, you need to create N virtual networks, each containing one NIC on machine number one and one NIC on machine number two.

For the NIC used to emulate N machines on machine one, its IP would be set in the 10.1.1.0/24 IP domain by test program automatically. (**You should not use any address from this region and N should not exceed 255**)
Set the IP of the NIC used to test program interactions to 10.1.0.1. On machine number two, set the IP of the NIC used to test program interactions to 10.1.0.2.

## Implement a simple L3 forward

You should implement your code in `yourcode.cpp` or `yourcode.c`.
We support both C and C++.

**We will use `yourcode.cpp` as defualt option, if you want to use C, you should just create a `yourcode.c` and delete the file named `yourcode.cpp`**

## Compile

In this section we will tell you how to compile your code and the test program.

### On Machine No.1

This step may consume time.
Because we will get gtest framework from `Github` automatically.

``` bash
git clone git@github.com:N2-Sys/Lab-DPDK-l3-forward.git
cd Lab-DPDK-l3-forward
mkdir build
cd build
cmake .. -G "Unix Makefiles" -DCONTROLLER=YES
make
```

### On Machine No.2

``` bash
git clone git@github.com:N2-Sys/Lab-DPDK-l3-forward.git
cd Lab-DPDK-l3-forward
mkdir build
cd build
cmake .. -G "Unix Makefiles" -DUSER=YES
make
```

## Run tests

In order to run the test, you need to start a process on each of the two machines.
Note that the processes start in a strict order of precedence.

First you need to start the test program on Machine No.1.

``` bash
cd build
./test testcase
```

Then start the router program on Machine No.2 to execute your code.

``` bash
cd build
./client <PARAMETERS TO DPDK>
```

The results and error messages will be displayed in the terminal on Machine No.1.
If the error is caused by your code causing the program to crash, then we will not be able to detect the cause of the error and you will need to check it yourself on Machine No.2.

## Practical scenario testing

You just need to connect the connected N **peer ports** of Machine No.2 to different virtual machines instead of Machine No.1.
And then start the router process on Machine No.2.

Execute the following command on Machine No.1 to start the interactive mode of the controller. In this mode, the test program will only assume the existence of a network port used to interact with the router.

``` bash
cd build
./test interactive
```

The following command is available:

* `add_rule`: `add_rule 10.0.0.0 24 1` adds `forward 10.0.0.0/24 to port 1` to route table
* `del_rule`: `del_rule 10.0.0.0 24` deletes entry in route table that matches `forward 10.0.0.0/24 to port *`

Then you can test the connectivity by pinging or by running iperf on different machines and other tools.

> Enjoy our lab
