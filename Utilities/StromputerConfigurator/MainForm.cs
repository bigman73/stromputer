using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO.Ports;

namespace StromputerConfig
{
    public partial class MainForm : Form
    {
        public MainForm()
        {
            InitializeComponent();
        }

        private System.IO.Ports.SerialPort _arduinoSerialPort;


        private void initSerialButton_Click(object sender, EventArgs e)
        {
            System.ComponentModel.IContainer components = new System.ComponentModel.Container();
            _arduinoSerialPort = new System.IO.Ports.SerialPort(components);
            _arduinoSerialPort.PortName = "COM16";
            _arduinoSerialPort.BaudRate = 9600;

            _arduinoSerialPort.Open();
            if (!_arduinoSerialPort.IsOpen)
            {
                Console.WriteLine("Oops");
                return;
            }

            // this turns on !
            _arduinoSerialPort.DtrEnable = true;

            // callback for text coming back from the arduino
            _arduinoSerialPort.DataReceived += OnReceived;

            // give it 2 secs to start up the sketch
            System.Threading.Thread.Sleep(3000);
        }

        private void OnReceived(object sender, SerialDataReceivedEventArgs c)
        {
            try
            {
                // write out text coming back from the arduino
                MethodInvoker invoker = new MethodInvoker(delegate()
                        { responseTextBox.Text += _arduinoSerialPort.ReadExisting(); });
   
                Invoke(invoker);
            }
            catch (Exception exc) 
            {
                Console.WriteLine(exc.ToString());
            }
        }



        private void sendIsAliveButton_Click(object sender, EventArgs e)
        {
            Console.WriteLine("sending StromputerAlive command");
            _arduinoSerialPort.Write("StromputerAlive?");
            System.Threading.Thread.Sleep(100);
        }

        private void sendTempButton_Click(object sender, EventArgs e)
        {
            Console.WriteLine("sending Temp? command");
            _arduinoSerialPort.Write("Temp?");
            System.Threading.Thread.Sleep(100);
        }
    }
}
