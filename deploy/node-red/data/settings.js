// Minimal Node-RED settings for the local development stack.
// Credential encryption is disabled so the InfluxDB token can live in a
// human-readable flows_cred.json checked into the repo (local-dev token only).
module.exports = {
    flowFile: "flows.json",
    credentialSecret: false,
    uiPort: process.env.PORT || 1880,
    logging: {
        console: {
            level: "info",
            metrics: false,
            audit: false
        }
    },
    exportGlobalContextKeys: false,
    functionGlobalContext: {},
    editorTheme: {
        projects: {
            enabled: false
        }
    }
};
