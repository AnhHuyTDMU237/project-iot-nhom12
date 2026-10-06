class AppConfig {
  // =========================================================
  // IP của máy tính đang chạy FastAPI
  // =========================================================
  //
  // Ví dụ:
  // http://192.168.137.1:8000
  //
  // SAU NÀY khi deploy backend:
  // https://api-ten-project.com
  //
  static const String apiBaseUrl =
      'http://192.168.137.1:8000';

  static const String deviceId = 'esp32-001';
}