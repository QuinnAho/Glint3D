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
export declare class Glint3D {
    private engineModule;
    private canvas;
    private config;
    constructor(config: Glint3DConfig);
    initialize(): Promise<void>;
    loadModel(path: string, name?: string): Promise<string>;
    duplicateObject(objectId: string): Promise<string>;
    removeObject(objectId: string): Promise<void>;
    setObjectTransform(objectId: string, transform: Partial<Transform>): Promise<void>;
    setObjectMaterial(objectId: string, material: Partial<Material>): Promise<void>;
    addLight(light: Omit<Light, 'id'>): Promise<string>;
    updateLight(lightId: string, updates: Partial<Light>): Promise<void>;
    removeLight(lightId: string): Promise<void>;
    setCamera(camera: Partial<Camera>): Promise<void>;
    setCameraPreset(preset: string): Promise<void>;
    render(options: RenderOptions): Promise<Uint8Array>;
    renderToFile(options: RenderOptions & {
        outputPath: string;
    }): Promise<void>;
    getScene(): Promise<Scene>;
    importScene(sceneData: string): Promise<void>;
    exportScene(): Promise<string>;
    setBackground(background: Background): Promise<void>;
    private executeIntent;
    private translateIntentToJsonOp;
    private executeJsonOp;
}
export default Glint3D;
