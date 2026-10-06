import 'package:flutter/material.dart';

import '../services/api_service.dart';

class ControlScreen extends StatefulWidget {
  const ControlScreen({super.key});

  @override
  State<ControlScreen> createState() =>
      _ControlScreenState();
}

class _ControlScreenState
    extends State<ControlScreen> {

  bool fan = false;
  bool pump = false;
  bool buzzer = false;
  bool light = false;
  bool door = false;

  bool sending = false;

  Future<void> sendCommand(
    String command,
    String device,
    bool value,
  ) async {
    setState(() {
      sending = true;
    });

    try {
      await ApiService.sendCommand(command);

      if (!mounted) return;

      setState(() {
        switch (device) {
          case 'fan':
            fan = value;
            break;

          case 'pump':
            pump = value;
            break;

          case 'buzzer':
            buzzer = value;
            break;

          case 'light':
            light = value;
            break;

          case 'door':
            door = value;
            break;
        }
      });

      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text(
            '$command thành công',
          ),
        ),
      );
    } catch (e) {
      if (!mounted) return;

      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text(
            'Lỗi: $e',
          ),
        ),
      );
    } finally {
      if (mounted) {
        setState(() {
          sending = false;
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text(
          'Điều khiển thiết bị',
        ),
      ),

      body: ListView(
        padding: const EdgeInsets.all(16),

        children: [

          _control(
            title: 'FAN',
            icon: Icons.air,
            value: fan,
            onChanged: (value) {
              sendCommand(
                value ? 'FAN_ON' : 'FAN_OFF',
                'fan',
                value,
              );
            },
          ),

          _control(
            title: 'PUMP',
            icon: Icons.water,
            value: pump,
            onChanged: (value) {
              sendCommand(
                value ? 'PUMP_ON' : 'PUMP_OFF',
                'pump',
                value,
              );
            },
          ),

          _control(
            title: 'BUZZER',
            icon: Icons.volume_up,
            value: buzzer,
            onChanged: (value) {
              sendCommand(
                value
                    ? 'BUZZER_ON'
                    : 'BUZZER_OFF',
                'buzzer',
                value,
              );
            },
          ),

          _control(
            title: 'LIGHT',
            icon: Icons.lightbulb,
            value: light,
            onChanged: (value) {
              sendCommand(
                value
                    ? 'LIGHT_ON'
                    : 'LIGHT_OFF',
                'light',
                value,
              );
            },
          ),

          _control(
            title: 'DOOR',
            icon: Icons.door_front_door,
            value: door,
            onChanged: (value) {
              sendCommand(
                value
                    ? 'DOOR_OPEN'
                    : 'DOOR_CLOSE',
                'door',
                value,
              );
            },
          ),

          if (sending)
            const Padding(
              padding: EdgeInsets.all(20),
              child: Center(
                child: CircularProgressIndicator(),
              ),
            ),
        ],
      ),
    );
  }

  Widget _control({
    required String title,
    required IconData icon,
    required bool value,
    required ValueChanged<bool> onChanged,
  }) {
    return Card(
      margin: const EdgeInsets.only(
        bottom: 12,
      ),

      child: SwitchListTile(
        secondary: Icon(icon),

        title: Text(
          title,
          style: const TextStyle(
            fontWeight: FontWeight.bold,
          ),
        ),

        subtitle: Text(
          value ? 'ON' : 'OFF',
        ),

        value: value,

        onChanged:
            sending ? null : onChanged,
      ),
    );
  }
}