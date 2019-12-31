# Smart Client

## Feature

### Client

- User Friendly Interface: If you key in unknown or invalid command, command line will feed back all commands you can use.

- Coool cmd-like style: Put a '$' symbol in front of every line, just like bash. If you have logged in, your username will also put before '$'. Example:  `eethan1$`

- Logout: Logout make you have choice to log in again instead of exiting. (However, it seems that the server haven't supported this feature)

### Server

- Thread-pool: enhance connection performance with reusable thread.

- Exception-Resist: If some client logout or just close socket, the server will still service other client insted of crashing.

- Reusable bind: If server crash, the bind port will be occupied for a while. This feature makes server reuse same port to listen possible.

- Good coding style: Split ugly code into magic.h.

## Requirement

- Compiling enrionment's OS is Arch Linux and kernel is 5.4.2-arch1-1. 
- It can also compile on department's workstation.
- It use standard C++14.

## Compile

- Dynamic link: `make`
- Static link: `make static`

## Run

- `./server -t <IP address> -p <port>`
- `./client -t <IP address> -p <port>`
