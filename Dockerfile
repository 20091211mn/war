# خادم لعب مخصص (بلا واجهة). البناء:  docker build -t warzone-server .
# التشغيل:  docker run -d --restart=always -p 9330:9330/udp warzone-server
FROM debian:12-slim
ARG GODOT=4.7.2-stable
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates curl unzip libfontconfig1 && rm -rf /var/lib/apt/lists/*
RUN curl -fsSL -o /tmp/g.zip https://github.com/godotengine/godot-builds/releases/download/${GODOT}/Godot_v${GODOT}_linux.x86_64.zip \
 && unzip /tmp/g.zip -d /usr/local/bin && mv /usr/local/bin/Godot_v${GODOT}_linux.x86_64 /usr/local/bin/godot && rm /tmp/g.zip
COPY godot /game
RUN godot --headless --path /game --import || true
ENV PORT=9330 MAXP=8 NBOTS=2
EXPOSE 9330/udp
CMD godot --headless --path /game -- --dedicated --port=$PORT --maxp=$MAXP --nbots=$NBOTS
