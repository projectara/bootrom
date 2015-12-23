Build:

The library has two configurations files;

defconfig:  Enable cross compile, testing and function decoration

config.mk:  Select crypto variables such as word length of computer, curve, 
            curve type and finite field size multiple

The current configuration is for Linux 64 bit with C25519 and RSA2048. 

To build:   make

In order to support multiple curves and RSA bit lengths then the library must
be built three times. The same function names would be present in the three 
libraries and obviously this would cause an error. The solution is to use the
preprocessor to change the name of the functions. The file Decorator.mk shows
the functions that are appended with the curve or RSA bit length choice. 

A run-time program can then be written which calls the correct curve or RSA bit 
length function. Two examples are given;

test_runtime_dev.c  Uses DecoratorRuntime.mk to change names of functions.
test_runtime.c      Fixed functions names

There is a script that builds these example which requires the word length of 
the computer as an input.

To build:   ./build.bsh 64

The build scripts support the Marvell 88MW300 SoC. The SDK is required

git clone https://github.com/marvell-iot/aws_starter_sdk.git

Please follow the instructions to build the Marvell libraries;

https://github.com/marvell-iot/aws_starter_sdk/wiki

When this is done then change the defconfig file;

CONFIG_ARM=y 

To build:   ./build.bsh 32

description:

In the ROM file (rom.c) are provide the elliptic curve constants. Several 
examples are provided there, and if you are willing to use one of these, 
simply select your curve of MCL_CHOICE.

Only some combinations of curve and curve type may be supported. The RSA 
bit length is constrained to be a multiple of the elliptic curve field size
 - see comments in config.mk for guidance).

Two example API files are provided, rsa.c which supports RSA signature
and encryption and ecdh.c which supports standard elliptic 
curve key exchange, digital signature and public key crypto. 
