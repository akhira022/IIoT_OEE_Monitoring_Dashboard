FROM nodered/node-red:3.1

# Palette modules pinned to the versions recorded in the project flow
# (node-red/flows_final_8.3v.json -> global-config.modules).
RUN npm install --no-audit --no-fund --no-update-notifier \
    node-red-dashboard@3.6.6 \
    node-red-contrib-influxdb@0.7.0
