/**
 * Glint3D Web SDK v1.0
 * Versioned intent surface for AI-maintainable interaction with Glint3D engine
 */

export interface Glint3DConfig {
  canvasId: string;
  enginePath?: string;
  assetRoot?: string;
  debug?: boolean;
}

export interface Scene {
  objects: SceneObject[];
  lights: Light[];
  camera: Camera;
  background?: Background;
}

export interface SceneObject {
  id: string;
  name: string;
  meshPath?: string;
  transform: Transform;
  material: Material;
  visible: boolean;
}

export interface Transform {
  position: [number, number, number];
  rotation: [number, number, number];
  scale: [number, number, number];
}

export interface Material {
  color: [number, number, number];
  roughness: number;
  metallic: number;
  ior: number;
  transmission: number;
  emissive?: [number, number, number];
}

export interface Light {
  id: string;
  type: 'point' | 'directional' | 'spot';
  position?: [number, number, number];
  direction?: [number, number, number];
  color: [number, number, number];
  intensity: number;
  enabled: boolean;
  // Spot light specific
  innerAngle?: number;
  outerAngle?: number;
}

export interface Camera {
  position: [number, number, number];
  target: [number, number, number];
  fov: number;
  near: number;
  far: number;
}

export interface Background {
  type: 'color' | 'skybox';
  value: [number, number, number] | string;
}

export interface RenderOptions {
  width: number;
  height: number;
  samples?: number;
  denoise?: boolean;
  outputPath?: string;
}

/**
 * Intent-based operations for AI interactions
 * These map to JSON Ops v2 internally but provide a cleaner interface
 */
export class Glint3D {
  private engineModule: any;
  private canvas: HTMLCanvasElement;
  private config: Glint3DConfig;

  constructor(config: Glint3DConfig) {
    this.config = config;
    this.canvas = document.getElementById(config.canvasId) as HTMLCanvasElement;
  }

  async initialize(): Promise<void> {
    // Initialize WASM engine
    // This would load the Emscripten module
  }

  // Scene Management Intents
  async loadModel(path: string, name?: string): Promise<string> {
    return this.executeIntent('load_model', { path, name });
  }

  async duplicateObject(objectId: string): Promise<string> {
    return this.executeIntent('duplicate_object', { objectId });
  }

  async removeObject(objectId: string): Promise<void> {
    return this.executeIntent('remove_object', { objectId });
  }

  async setObjectTransform(objectId: string, transform: Partial<Transform>): Promise<void> {
    return this.executeIntent('set_object_transform', { objectId, transform });
  }

  async setObjectMaterial(objectId: string, material: Partial<Material>): Promise<void> {
    return this.executeIntent('set_object_material', { objectId, material });
  }

  // Lighting Intents
  async addLight(light: Omit<Light, 'id'>): Promise<string> {
    return this.executeIntent('add_light', light);
  }

  async updateLight(lightId: string, updates: Partial<Light>): Promise<void> {
    return this.executeIntent('update_light', { lightId, ...updates });
  }

  async removeLight(lightId: string): Promise<void> {
    return this.executeIntent('remove_light', { lightId });
  }

  // Camera Intents
  async setCamera(camera: Partial<Camera>): Promise<void> {
    return this.executeIntent('set_camera', camera);
  }

  async setCameraPreset(preset: string): Promise<void> {
    return this.executeIntent('set_camera_preset', { preset });
  }

  // Rendering Intents
  async render(options: RenderOptions): Promise<Uint8Array> {
    return this.executeIntent('render', options);
  }

  async renderToFile(options: RenderOptions & { outputPath: string }): Promise<void> {
    return this.executeIntent('render_to_file', options);
  }

  // Scene State
  async getScene(): Promise<Scene> {
    return this.executeIntent('get_scene', {});
  }

  async importScene(sceneData: string): Promise<void> {
    return this.executeIntent('import_scene', { data: sceneData });
  }

  async exportScene(): Promise<string> {
    return this.executeIntent('export_scene', {});
  }

  // Background
  async setBackground(background: Background): Promise<void> {
    return this.executeIntent('set_background', background);
  }

  // Internal intent execution
  private async executeIntent(intent: string, params: any): Promise<any> {
    // This would translate intents to JSON Ops and execute via engine
    const jsonOp = this.translateIntentToJsonOp(intent, params);
    return this.executeJsonOp(jsonOp);
  }

  private translateIntentToJsonOp(intent: string, params: any): any {
    // Intent -> JSON Op translation layer
    // This provides AI-friendly abstraction over raw JSON Ops
    return {
      op: intent,
      ...params
    };
  }

  private async executeJsonOp(jsonOp: any): Promise<any> {
    // Execute via Emscripten bridge
    if (this.engineModule && this.engineModule.app_apply_ops_json) {
      const result = this.engineModule.app_apply_ops_json(JSON.stringify(jsonOp));
      return JSON.parse(result);
    }
    throw new Error('Engine not initialized');
  }
}

export default Glint3D;