package top.zxff.nativeblereader;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.widget.Toast;

import androidx.annotation.Discouraged;
import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;

import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.util.HashMap;
import java.util.Set;
import java.util.UUID;

import dalvik.system.PathClassLoader;

public class BleReader {

    @Discouraged(message = "this method should be rewritten in C++ with JNI to load this java code.")
    public static void LoadJavaLibrary(String path) throws ClassNotFoundException {
        new PathClassLoader(path,
                Class.forName("com.unity3d.player.UnityPlayer").getClassLoader())
            .loadClass("top.zxff.nativeblereader.BleReader");
    }

    public native void InformNativeDevice(String macAddr, String deviceName);
    public native void OnDeviceData(String macAddr, int heartRate, long energy);


    public boolean IsDeviceSelected(String macAddr){
        return BleDevices.containsKey(macAddr) && BleDevices.get(macAddr).selected;
    }


    Context context;
    public BleReader(){
        this.context = GetActivity();
        if(this.context == null){
            throw new RuntimeException("The context is nullptr");
        }
    }

    class DeviceStatus{
        boolean selected = false;

        BluetoothDevice dev;
        BluetoothGattCb cb;

        ////////////////////////////////////
        boolean serviceDiscovered = false;
        public DeviceStatus(BluetoothDevice dev){
            this.dev = dev;
        }
        @SuppressLint("MissingPermission")
        public boolean Toggle(boolean selected){
            if(this.selected == selected)
                return false;
            this.selected = selected;
            if(selected){
                //turn on
                cb = new BluetoothGattCb();
                cb.gatt = this.dev.connectGatt(context, true,cb);
            }else{
                //turn off
                cb.close();
                cb = null;
            }
            return true;
        }

        //As Google documented, the Callback happens in a background thread. Good!
        class BluetoothGattCb extends BluetoothGattCallback {
            final static String HEART_UUID = "00002a37-0000-1000-8000-00805f9b34fb";

            BluetoothGatt gatt;
            @SuppressLint("MissingPermission")
            public void close(){
                serviceDiscovered = false;
                this.gatt.close();
                this.gatt = null;
            }
            /*
            Why we need this variable called useLatestHandleGatt:
                Some device will use the new api : onCharacteristicChanged(gatt, chara, values)
                However, this api is never called in quest 2 device.
                Quest 2 use onCharacteristicChanged(gatt, chara) instead.
                The old api is deprecated in API LEVEL 33, and I'm not sure if it will called in latest device.
                So the stupid variable here to prevent duplicate data.
             */
            boolean useLatestHandleGatt = false;
            private void handleGatt(@NonNull BluetoothGatt gatt, @NonNull BluetoothGattCharacteristic characteristic){
                if (HEART_UUID.equals(characteristic.getUuid().toString())) {
                    int flag = characteristic.getProperties();

                    int format = -1;
                    if ((flag & 0x01) != 0) {
                        format = BluetoothGattCharacteristic.FORMAT_UINT16;
                    } else {
                        format = BluetoothGattCharacteristic.FORMAT_UINT8;
                    }
                    int heartRate = characteristic.getIntValue(format, 1);

                    long energy = 0;
                    if((flag & 0x8) != 0){
                        //energy
                        int offset = (flag & 1) + 2;
                        energy = characteristic.getIntValue(BluetoothGattCharacteristic.FORMAT_UINT16, offset);
                    }
                    OnDeviceData(dev.getAddress(), heartRate, energy);
                }
            }

            @Override
            public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
                super.onCharacteristicChanged(gatt, characteristic);
                if(useLatestHandleGatt)
                    return;
                handleGatt(gatt, characteristic);
            }


            @Override
            public void onCharacteristicChanged(@NonNull BluetoothGatt gatt, @NonNull BluetoothGattCharacteristic characteristic, @NonNull byte[] value) {
                super.onCharacteristicChanged(gatt, characteristic, value);
                useLatestHandleGatt = true;
                handleGatt(gatt, characteristic);
            }

            @SuppressLint("MissingPermission")
            @Override
            public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
                super.onConnectionStateChange(gatt, status, newState);
                if (newState == BluetoothProfile.STATE_CONNECTED) {
                    gatt.discoverServices();
                }
            }

            @SuppressLint("MissingPermission")
            @Override
            public void onServicesDiscovered(BluetoothGatt gatt, int status) {
                super.onServicesDiscovered(gatt, status);
                if (gatt != this.gatt)
                    return;
                for (BluetoothGattService serv : gatt.getServices()) {
                    for (BluetoothGattCharacteristic ch : serv.getCharacteristics()) {
                        if (HEART_UUID.equals(ch.getUuid().toString())) {
                            gatt.setCharacteristicNotification(ch, true);
                            BluetoothGattDescriptor descriptor = ch.getDescriptor(UUID.fromString("00002902-0000-1000-8000-00805f9b34fb"));
                            descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                            gatt.writeDescriptor(descriptor);
                            serviceDiscovered = true;
                        }
                    }
                }
            }
        }

    }

    /* mac -> DeviceStatus */
    HashMap<String, DeviceStatus> BleDevices = new HashMap<>();
    public void BleStart(){
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S &&
                ActivityCompat.checkSelfPermission(context, "android.permission.BLUETOOTH_CONNECT") != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions((Activity)context, new String[]{"android.permission.BLUETOOTH_CONNECT"}, 1);
            return;
        }
        Set<BluetoothDevice> deviceSet = BluetoothAdapter.getDefaultAdapter().getBondedDevices();
        for (BluetoothDevice bluetoothDevice : deviceSet) {
            if(!BleDevices.containsKey(bluetoothDevice.getAddress())){
                BleDevices.put(bluetoothDevice.getAddress(),
                        new DeviceStatus(bluetoothDevice));
            }
            InformNativeDevice(bluetoothDevice.getAddress(), bluetoothDevice.getName());
        }
    }

    public boolean BleToggle(String macAddress, boolean selected){
        return BleDevices.containsKey(macAddress) && BleDevices.get(macAddress).Toggle(selected);
    }

    private static Context GetActivity(){
        try {
            return (Context)Class
                    .forName("com.unity3d.player.UnityPlayer")
                    .getField("currentActivity")
                    .get(null);
        } catch (NoSuchFieldException | IllegalAccessException | ClassNotFoundException e) {
            throw new RuntimeException(e);
        }
    }
}
