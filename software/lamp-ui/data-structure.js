export default {
  lamp: {
    name: 'Snafu',
    brightness: 80,
    animations: ['rotating_stars', 'rainbow_stripes'],
    disposition: 'moody',
    homeMode: false,
  },
  shade: {
    leds: 18,
    colors: ['#673a8311', '#672CA847'],
    knockout: [
      { p: 4, b: 0.5 },
      { p: 5, b: 0.75 },
    ],
  },
  base: {
    leds: 24,
    colors: ['#7c4400ff', '#9B2E00C5'],
  },
  reactions: [
    { name: 'century', reaction: 'aloof' },
    { name: 'vamp', reaction: 'friendly' },
    { name: 'Draper', reaction: 'horney' },
  ],
  stage: {
    enabled: false,
    channel: 'random',
  },
}
