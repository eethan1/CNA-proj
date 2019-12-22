# Smart Client

## Feature

- User Friendly Interface: If you key in unknown or invalid command, command line will feed back all commands you can use.

- Coool cmd-like style: Put a '$' symbol in front of every line, just like bash. If you have logged in, your username will also put before '$'. Example:  `eethan1$`

- Logout: Logout make you have choice to log in again instead of exiting. (However, it seems that the server haven't supported this feature)

## Requirement

- Compiling enrionment's OS is Arch Linux and kernel is 5.4.2-arch1-1. 
- It can also compile on department's workstation.

## Compile

- Dynamic link: `make`
- Static link: `make static`
- Revised version: `make revised`. This version comform to revised TA program (number of account online) if exists.
- Revised static link: `make staticR`


## Run

- `./client -t <IP address> -p <port>`
