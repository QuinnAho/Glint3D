/**
 * Glint3D Web SDK v1.0
 * Versioned intent surface for AI-maintainable interaction with Glint3D engine
 */
/**
 * Intent-based operations for AI interactions
 * These map to JSON Ops v2 internally but provide a cleaner interface
 */
export class Glint3D {
    constructor(config) {
        this.config = config;
        this.canvas = document.getElementById(config.canvasId);
    }
    async initialize() {
        // Initialize WASM engine
        // This would load the Emscripten module
    }
    // Scene Management Intents
    async loadModel(path, name) {
        return this.executeIntent('load_model', { path, name });
    }
    async duplicateObject(objectId) {
        return this.executeIntent('duplicate_object', { objectId });
    }
    async removeObject(objectId) {
        return this.executeIntent('remove_object', { objectId });
    }
    async setObjectTransform(objectId, transform) {
        return this.executeIntent('set_object_transform', { objectId, transform });
    }
    async setObjectMaterial(objectId, material) {
        return this.executeIntent('set_object_material', { objectId, material });
    }
    // Lighting Intents
    async addLight(light) {
        return this.executeIntent('add_light', light);
    }
    async updateLight(lightId, updates) {
        return this.executeIntent('update_light', { lightId, ...updates });
    }
    async removeLight(lightId) {
        return this.executeIntent('remove_light', { lightId });
    }
    // Camera Intents
    async setCamera(camera) {
        return this.executeIntent('set_camera', camera);
    }
    async setCameraPreset(preset) {
        return this.executeIntent('set_camera_preset', { preset });
    }
    // Rendering Intents
    async render(options) {
        return this.executeIntent('render', options);
    }
    async renderToFile(options) {
        return this.executeIntent('render_to_file', options);
    }
    // Scene State
    async getScene() {
        return this.executeIntent('get_scene', {});
    }
    async importScene(sceneData) {
        return this.executeIntent('import_scene', { data: sceneData });
    }
    async exportScene() {
        return this.executeIntent('export_scene', {});
    }
    // Background
    async setBackground(background) {
        return this.executeIntent('set_background', background);
    }
    // Internal intent execution
    async executeIntent(intent, params) {
        // This would translate intents to JSON Ops and execute via engine
        const jsonOp = this.translateIntentToJsonOp(intent, params);
        return this.executeJsonOp(jsonOp);
    }
    translateIntentToJsonOp(intent, params) {
        // Intent -> JSON Op translation layer
        // This provides AI-friendly abstraction over raw JSON Ops
        return {
            op: intent,
            ...params
        };
    }
    async executeJsonOp(jsonOp) {
        // Execute via Emscripten bridge
        if (this.engineModule && this.engineModule.app_apply_ops_json) {
            const result = this.engineModule.app_apply_ops_json(JSON.stringify(jsonOp));
            return JSON.parse(result);
        }
        throw new Error('Engine not initialized');
    }
}
export default Glint3D;
