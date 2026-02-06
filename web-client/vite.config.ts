import { defineConfig } from 'vite'
import { svelte, vitePreprocess } from '@sveltejs/vite-plugin-svelte'
import viteCompression from 'vite-plugin-compression';

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [
    svelte({
      preprocess: vitePreprocess()
    }),
    viteCompression({
        algorithm: 'gzip',
        ext: '.gz',
        deleteOriginFile: true,
        threshold: 0,
        filter: /\.(js|mjs|json|css|html|svg)$/i
    })
  ],
  build: {
    outDir: '../data',
    emptyOutDir: true,
  }
})
