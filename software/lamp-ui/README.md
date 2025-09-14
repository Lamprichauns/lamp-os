# Lamp OS Color Picker

A mobile-first Vue.js color picker component with hexww (hex + warm white) support. Built with Vue 3, TypeScript, and Vitest.

## Features

- **Hexww Support**: Supports 8-digit hex values with warm white component (e.g., `#FF0000FF`)
- **Mobile-First Design**: Responsive design that works great on mobile devices
- **Interactive Dialog**: Click-to-open color picker with sliders for RGBA and warm white
- **Real-time Updates**: Changes to sliders update the hex value in real-time
- **Background Shading**: Modal dialog with clickable overlay to close
- **TypeScript Support**: Fully typed with TypeScript
- **Comprehensive Testing**: Unit tests with Vitest

## Color Format

The color picker supports the hexww format:

- **Standard Hex**: 6-digit hex values (e.g., `#FF0000`)
- **Hexww**: 8-digit hex values with warm white component (e.g., `#FF0000FF`)

The format is: `#RRGGBBWW` where:

- `RR` = Red component (00-FF)
- `GG` = Green component (00-FF)
- `BB` = Blue component (00-FF)
- `WW` = Warm White component (00-FF)

## Installation

```bash
# Clone the repository
git clone <repository-url>
cd software/lamp-ui

# Install dependencies
npm ci

# Start development server
npm run dev

# Run tests
npm run test:unit

# Build for production
npm run build
```

## Usage

### Basic Usage

```vue
<template>
  <ColorPicker v-model="selectedColor" />
</template>

<script setup>
import { ref } from 'vue'
import ColorPicker from '@/components/ColorPicker.vue'

const selectedColor = ref('#FF0000FF')
</script>
```

### Component Props

| Prop         | Type     | Default | Description           |
| ------------ | -------- | ------- | --------------------- |
| `modelValue` | `string` | `''`    | The hexww color value |

### Component Events

| Event               | Payload  | Description                          |
| ------------------- | -------- | ------------------------------------ |
| `update:modelValue` | `string` | Emitted when the color value changes |

## Development

### Project Structure

```
src/
├── components/
│   ├── ColorPicker.vue          # Main color picker component
│   └── __tests__/
│       └── ColorPicker.test.ts  # Component tests
├── views/
│   └── HomeView.vue             # Demo page
├── App.vue                      # Root component
└── main.ts                      # Application entry point
```

### Running Tests

```bash
# Run all tests
npm run test:unit

# Run tests in watch mode
npm run test:unit -- --watch
```

### Building

```bash
# Build for production
npm run build

# Preview production build
npm run preview
```

### Uploading to your board

Plug your lamp board into a usb port

In VSCode, from the `lamp-os` project, navigate to `Pioarduino > Quick Access > Miscellaneous > pioarduino core CLI`

This will bring up a window in the correct environment to upload

```bash
cd ../lamp-ui
npm ci
npm run build:upload
```

This process will build a new .spiffs partition and replace it onboard your esp32.

## Features in Detail

### Color Picker Dialog

- **Color Preview**: Large preview of the selected color
- **Hex Input**: Direct hex value input with validation
- **RGBA Sliders**: Individual sliders for red, green, blue, and warm white
- **Real-time Updates**: All changes update the hex value immediately
- **Click Outside to Close**: Click the overlay or X button to close

### Mobile Responsiveness

- Optimized for touch interactions
- Responsive layout that adapts to screen size
- Touch-friendly sliders and buttons
- Proper viewport handling

### Accessibility

- Proper ARIA labels
- Keyboard navigation support
- Focus management
- Screen reader friendly

## Example Colors

The demo page includes several example colors:

- **Pure Red**: `#FF0000FF`
- **Pure Green**: `#00FF00FF`
- **Pure Blue**: `#0000FFFF`
- **Warm White**: `#000000FF`
- **Purple**: `#800080FF`
- **Orange**: `#FF8000FF`
- **Cyan**: `#00FFFFFF`
- **Magenta**: `#FF00FFFF`

## Technologies Used

- **Vue 3**: Progressive JavaScript framework
- **TypeScript**: Type-safe JavaScript
- **Vite**: Fast build tool and dev server
- **Vitest**: Unit testing framework
- **Vue Router**: Client-side routing
- **Pinia**: State management
- **ESLint**: Code linting
- **Prettier**: Code formatting

## Browser Support

- Chrome (latest)
- Firefox (latest)
- Safari (latest)
- Edge (latest)
- Mobile browsers (iOS Safari, Chrome Mobile)

## License

MIT License - see LICENSE file for details.
