FROM ubuntu:latest
RUN apt update && apt install -y sudo dotnet-sdk-8.0  net-tools  git wget iputils-ping openssh-server
WORKDIR /App
RUN adduser jmca
RUN echo -n 'jmca:pass' | chpasswd
RUN echo -n 'root:pass' | chpasswd
RUN mkdir -p /var/run/sshd
RUN git clone https://github.com/isaacdenny/JmcaC2.git
WORKDIR ./JmcaC2/JmcaC2/
RUN dotnet run
# We should run a server in here
CMD ["/usr/sbin/sshd", "-D"]

# docker run -d --name jmcac2 --rm -p 2000:22 jmcac2:latest