FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ \
    make \
    pkg-config \
    libcurl4-openssl-dev \
    nlohmann-json3-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY src ./src

RUN g++ -std=c++17 -O2 -Wall -Wextra src/main.cpp -o recovery \
    $(pkg-config --cflags --libs libcurl)

CMD ["./recovery"]
