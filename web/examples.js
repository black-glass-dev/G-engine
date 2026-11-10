// G-Engine — web/examples.js
// Sentences offered under the editor ("try adding these"), per object and per language.
// tools/web_test.mjs compiles every one of them, so a broken example fails the test.

export const examples = {
  'খেলোয়াড়': {
    bn: [
      'মাটিতে থাকলে আলোর রং নীল করো, নাহলে আলোর রং লাল করো',
      'শুরুতে গতি ৭ করো',
      'শুরুতে লাফের উচ্চতা ৪ করো',
      'কাঁটা ছুঁলে পর্দা কাঁপাও',
      'শুরুতে "চলো শুরু করি!" বলো',
      'অ্যাকশন বোতাম চাপলে আলো নেভাও\nঅ্যাকশন বোতাম ছাড়লে আলো জ্বালাও',
      'অ্যাকশন বোতাম চাপলে "খোলো" বার্তা পাঠাও',
    ],
    en: [
      'if in the air, make the light red, else make the light blue',
      'on start, set speed to 7',
      'on start, set jump height to 4',
      'when touching spikes, shake the screen',
      'on start, say "let\'s go!"',
      'when the action button is pressed, turn the light off\nwhen the action button is released, turn the light on',
      'when the action button is pressed, send the message "open"',
    ],
  },
  'প্রহরী': {
    bn: [
      'খেলোয়াড় ৪ মিটারের মধ্যে এলে আলোর রং হলুদ করো, নাহলে আলোর রং লাল করো',
      'প্রতি ৩ সেকেন্ডে লাফ দাও',
      'শুরুতে গতি ৪ করো',
      'শুরুতে আকার ১.৫ করো',
    ],
    en: [
      'when the player is within 4 meters, make the light yellow, else make the light red',
      'every 3 seconds, jump',
      'on start, set speed to 4',
      'on start, set size to 1.5',
    ],
  },
  'কয়েন': {
    bn: ['শুরুতে আলোর রং সবুজ করো', 'বারবার:\n    উপরে ১ মিটার যাও\n    নিচে ১ মিটার যাও'],
    en: ['on start, make the light green', 'forever:\n    go up 1 meter\n    go down 1 meter'],
  },
  'দরজা': {
    bn: ['"খোলো" বার্তা পেলে মুছে যাও', 'প্রতি ৪ সেকেন্ডে "টক টক" বলো'],
    en: ['when the message "open" arrives, disappear', 'every 4 seconds, say "knock knock"'],
  },
  'বাতি': {
    bn: [
      'খেলোয়াড় ৯ মিটারের মধ্যে এলে আলো জ্বালাও, নাহলে আলো নেভাও',
      'শুরুতে আলোর রং সবুজ করো',
      'শুরুতে উজ্জ্বলতা ৫ করো',
      'শুরুতে গতি ৬ করো',
    ],
    en: [
      'when the player is within 9 meters, turn the light on, else turn the light off',
      'on start, make the light green',
      'on start, set brightness to 5',
      'on start, set speed to 6',
    ],
  },
  'বাক্স': {
    bn: ['প্রতি ৩ সেকেন্ডে লাফ দাও', 'শুরুতে আকার ১.৫ করো', 'খেলোয়াড়কে ছুঁলে "আস্তে!" বলো'],
    en: ['every 3 seconds, jump', 'on start, set size to 1.5', 'when touching the player, say "easy!"'],
  },
};
