# Dockerfile для STM32 симулятора и оркестратора
# Multi-stage build для минимизации размера образа

# ============================================
# Stage 1: Сборка C-симулятора
# ============================================
FROM ubuntu:22.04 AS simulator-builder

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Копируем исходники симулятора
COPY core/ ./core/
COPY CMakeLists.txt ./
COPY test/ ./test/

# Собираем симулятор
RUN mkdir build && cd build && \
    cmake .. && \
    make && \
    cp stm32_sim /stm32_sim

# ============================================
# Stage 2: Сборка Go-оркестратора
# ============================================
FROM golang:1.21 AS orchestrator-builder

WORKDIR /build

# Копируем go.mod и загружаем зависимости
COPY orchestrator/go.mod ./orchestrator/
WORKDIR /build/orchestrator
RUN go mod download || true

# Копируем исходники оркестратора
COPY orchestrator/ ./

# Собираем оркестратор
RUN CGO_ENABLED=0 GOOS=linux go build -a -installsuffix cgo -o /orchestrator .

# ============================================
# Stage 3: Финальный образ
# ============================================
FROM debian:bookworm-slim

# Устанавливаем минимальные зависимости для симулятора
RUN apt-get update && apt-get install -y \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Копируем скомпилированные бинарники
COPY --from=simulator-builder /stm32_sim /app/stm32_sim
COPY --from=orchestrator-builder /orchestrator /app/orchestrator

# Создаём директорию для временных файлов
RUN mkdir -p /tmp/firmware

WORKDIR /app

# Переменные окружения
ENV REDIS_ADDR=localhost:6379
ENV TASK_QUEUE=simulator:tasks
ENV RESULT_QUEUE=simulator:results

# Экспортируем порт для метрик (если понадобится)
EXPOSE 8080

# Запускаем оркестратор
ENTRYPOINT ["/app/orchestrator"]
CMD ["--mode", "queue"]
