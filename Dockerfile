FROM gcc:14 AS build

ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

WORKDIR /app

COPY . .

RUN gcc -Wall -Wextra -O2 main.c -o sistema

FROM debian:12-slim

WORKDIR /app
COPY --from=build /app/sistema /app/sistema
COPY --from=build /app/cursos.txt /app/cursos.txt
COPY --from=build /app/alunos.txt /app/alunos.txt

RUN apt-get update && apt-get install -y \
    locales \
    && rm -rf /var/lib/apt/lists/*

RUN sed -i 's/# pt_BR.UTF-8 UTF-8/pt_BR.UTF-8 UTF-8/' /etc/locale.gen && \
    locale-gen

ENV LANG=pt_BR.UTF-8  
ENV LC_ALL=pt_BR.UTF-8  

CMD ["./sistema"]

