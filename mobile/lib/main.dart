import 'package:flutter/material.dart';

import 'screens/home_screen.dart';
import 'screens/camera_screen.dart';

void main() {
  runApp(
    const FireGasApp(),
  );
}

class FireGasApp extends StatelessWidget {
  const FireGasApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,

      title: 'Fire Gas System',

      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(
          seedColor: Colors.red,
        ),

        useMaterial3: true,
      ),

      // =====================================================
      // ROUTES
      // =====================================================

      routes: {
        '/': (context) => const HomeScreen(),

        '/camera': (context) => const CameraScreen(),
      },

      initialRoute: '/',
    );
  }
}