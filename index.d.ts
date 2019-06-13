export type SIGNAL = "SIGSEGV" | string;

export type CrashCallback = (signal: SIGNAL, address: string, stack: string[]) => any;

export function registerHandler(): void;
export function registerHandler(file: string): void;
export function registerHandler(file: string, callback: CrashCallback): void;

export function causeSegfault(): void;