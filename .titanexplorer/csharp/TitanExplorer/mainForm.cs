using System;
using System.Management;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO.Ports;
using TitanExplorer.Properties;
using DarkUI.Forms;


namespace TitanExplorer
{
    public partial class mainForm : DarkForm
    {
        logForm logWindow;
        List<graphForm> graphWindow;
        int savedHeight;

        string plotCheckedListBoxEditTextboxSavedText;

        int titan_receiveState;
        byte[] titan_receiveCommand;
        int titan_receiveCommandLength;
        int titan_receiveWaitLength;

        int titan_receiveBytes;
        int titan_lastReceiveBytes;

        public mainForm()
        {
            InitializeComponent();
        }

        private void mainForm_Load(object sender, EventArgs e)
        {
            //Properties.Settings.Default.Reset();

            // Set window location
            if (Settings.Default.mainWindowLocation != null)
            {
                this.Location = Settings.Default.mainWindowLocation;
            }

            // Set window size
            if (Settings.Default.mainWindowSize != null)
            {
                if ((Settings.Default.mainWindowSize.Height > 100) && (Settings.Default.mainWindowSize.Width > 100))
                {
                    this.Size = Settings.Default.mainWindowSize;
                }
            }

            bindLogCheckBox.Checked = Settings.Default.bindFromLog;
            bindGraphCheckBox.Checked = Settings.Default.bindFromGraph;

            // Set plot signals
            plotCheckedListBox.Items.Clear();
            for (int i = 0; i < Settings.Default.plotSignals.Count; i++)
            {
                plotCheckedListBox.Items.Add("[" + i.ToString() + "] " + Settings.Default.plotSignals[i]);
            }

            logWindow = new logForm();
            logWindow.FormClosing += logForm_FormClosing;
            logWindow.VisibleChanged += logForm_VisibleChanged;
            logWindow.Activated += logForm_Activated;
            logWindow.logTextBox.KeyDown += bindFromLog_KeyDown;
            logWindow.Visible = Settings.Default.logWindowOpened;
            logWindow.logTimestampCheckBox.Checked = Settings.Default.logWindowTimestamp;
            logWindow.alwaysOnTopCheckBox.Checked = Settings.Default.logWindowAlwaysOnTop;

            // Set window location
            if (Settings.Default.logWindowLocation != null)
            {
                logWindow.Location = Settings.Default.logWindowLocation;
            }

            // Set window size
            if (Settings.Default.logWindowSize != null)
            {
                if ((Settings.Default.logWindowSize.Height > 100) && (Settings.Default.logWindowSize.Width > 100))
                {
                    logWindow.Size = Settings.Default.logWindowSize;
                }
            }

            graphWindow = new List<graphForm>();
            logToolStripStatusLabel.Text = "application started";

            baudToolStripDropDownButton.Text = Settings.Default.baudRate;
            connectToolStripSplitButton.Text = Settings.Default.comPort;

            titan_receiveState = 0;
            titan_receiveCommand = new byte[65536 + 128];
            titan_receiveCommandLength = 0;
            titan_receiveWaitLength = 0;

            titan_receiveBytes = 0;
            titan_lastReceiveBytes = 0;

            connectToolStripSplitButton_DropDownOpening(sender, e); //initialize
        }

        private void mainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            // Copy window location to app settings
            Settings.Default.mainWindowLocation = this.Location;

            // Copy window size to app settings
            if (this.WindowState == FormWindowState.Normal)
            {
                Settings.Default.mainWindowSize = this.Size;
            }
            else
            {
                Settings.Default.mainWindowSize = this.RestoreBounds.Size;
            }

            // Settings.Default.plotSignals managed on plotCheckedList

            Settings.Default.bindFromLog = bindLogCheckBox.Checked;
            Settings.Default.bindFromGraph = bindGraphCheckBox.Checked;

            Settings.Default.logWindowTimestamp = logWindow.logTimestampCheckBox.Checked;
            Settings.Default.logWindowAlwaysOnTop = logWindow.alwaysOnTopCheckBox.Checked;
            Settings.Default.logWindowOpened = logWindow.Visible;

            Settings.Default.logWindowLocation = logWindow.Location;
            if (logWindow.WindowState == FormWindowState.Normal)
            {
                Settings.Default.logWindowSize = logWindow.Size;
            }
            else
            {
                Settings.Default.logWindowSize = logWindow.RestoreBounds.Size;
            }

            Settings.Default.baudRate = baudToolStripDropDownButton.Text;
            Settings.Default.comPort = connectToolStripSplitButton.Text;

            // Save settings
            Settings.Default.Save();
        }

        private void mainForm_Activated(object sender, EventArgs e)
        {
            statusStrip.Parent.Height -= statusStrip.Height;
            Controls.Add(statusStrip);
            Height = savedHeight + statusStrip.Height;
        }

        private void mainForm_Resize(object sender, EventArgs e)
        {
            if (WindowState != FormWindowState.Minimized)
            {
                savedHeight = Height;
            }
        }

        private void logForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            logForm window = (logForm)sender;
            e.Cancel = true;
            window.Hide();
        }

        private void logForm_VisibleChanged(object sender, EventArgs e)
        {
            logToolStripMenuItem.Checked = logWindow.Visible;
        }

        private void logForm_Activated(object sender, EventArgs e)
        {
            logForm window = (logForm)sender;
            statusStrip.Parent.Height -= statusStrip.Height;
            window.Controls.Add(statusStrip);
            window.Height = window.savedHeight + statusStrip.Height;
        }

        private void graphForm_FormClosed(object sender, FormClosedEventArgs e)
        {
            graphForm graph = (graphForm)sender;
            graphWindow.Remove(graph);
            windowToolStripMenuItem.DropDownItems.RemoveByKey("graphWindow" + graph.graph_id.ToString() + "ToolStripMenuItem");

            if (graphWindow.Count == 0)
            {
                ToolStripMenuItem mi = new ToolStripMenuItem("No graphs opened") { Name = "graphWindowNoneToolStripMenuItem" };
                mi.Enabled = false; ;
                windowToolStripMenuItem.DropDownItems.Add(mi);
            }

            logToolStripStatusLabel.Text = "graph window closed";
        }

        private void graphForm_Activated(object sender, EventArgs e)
        {
            graphForm graph = (graphForm)sender;
            statusStrip.Parent.Height -= statusStrip.Height;
            graph.Controls.Add(statusStrip);
            graph.Height = graph.savedHeight + statusStrip.Height;
        }

        private void addPlotButton_Click(object sender, EventArgs e)
        {
            graphForm graph = new graphForm();
            graphWindow.Add(graph);

            graph.graph_id = graphWindow.IndexOf(graph);
            graph.FormClosed += graphForm_FormClosed;
            graph.Activated += graphForm_Activated;
            graph.KeyDown += bindFromGraph_KeyDown;
            graph.chart.KeyDown += bindFromGraph_KeyDown;
            graph.Text += " Window " + graph.graph_id.ToString();
            graph.Show();

            for (int i = 0; i < plotCheckedListBox.Items.Count; i++)
            {
                if (plotCheckedListBox.GetItemCheckState(i) == CheckState.Checked)
                {
                    graph.addSignal(i, Settings.Default.plotSignals[i]);
                    plotCheckedListBox.SetItemCheckState(i, CheckState.Unchecked);
                }
            }
            
            logToolStripStatusLabel.Text = "graph window opened";

            if(graph.graph_id == 0)
            {
                windowToolStripMenuItem.DropDownItems.RemoveByKey("graphWindowNoneToolStripMenuItem");
            }

            ToolStripMenuItem mi = new ToolStripMenuItem("Graph " + graph.graph_id.ToString()) { Name = "graphWindow" + graph.graph_id.ToString() + "ToolStripMenuItem" };
            mi.Click += (s, ex) => graph.Focus();
            windowToolStripMenuItem.DropDownItems.Add(mi);
        }

        private void closePlotsButton_Click(object sender, EventArgs e)
        {
            for (int i = graphWindow.Count - 1; i >= 0; i--)
            {
                graphWindow[i].Close();
            }
            logToolStripStatusLabel.Text = "closed all graphs";
        }

        private void plotCheckedListBox_DoubleClick(object sender, EventArgs e)
        {
            plotCheckedListBoxEditTextbox.Location = new Point(plotCheckedListBox.GetItemRectangle(plotCheckedListBox.SelectedIndex).Location.X + SystemInformation.MenuCheckSize.Width + 3, plotCheckedListBox.GetItemRectangle(plotCheckedListBox.SelectedIndex).Location.Y);
            plotCheckedListBoxEditTextbox.Text = Settings.Default.plotSignals[plotCheckedListBox.SelectedIndex];
            plotCheckedListBoxEditTextboxSavedText = plotCheckedListBoxEditTextbox.Text;
            plotCheckedListBoxEditTextbox.Visible = true;
            plotCheckedListBoxEditTextbox.Focus();
        }

        private void plotCheckedListBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (plotCheckedListBox.SelectedIndex != -1)
            {
                int keyValue = e.KeyValue;
                if (keyValue >= (int)Keys.A && keyValue <= (int)Keys.Z)
                {
                    plotCheckedListBox_DoubleClick(sender, e);
                    
                    if (!e.Shift && keyValue >= (int)Keys.A && keyValue <= (int)Keys.Z)
                    {
                        plotCheckedListBoxEditTextbox.Text = ((char)(keyValue + 32)).ToString();
                    }
                    else
                    {
                        plotCheckedListBoxEditTextbox.Text = ((char)keyValue).ToString();
                    }

                    plotCheckedListBoxEditTextbox.SelectionStart = plotCheckedListBoxEditTextbox.Text.Length;
                    plotCheckedListBoxEditTextbox.SelectionLength = 0;
                }
            }
        }

        private void plotCheckedListBoxEditTextbox_Leave(object sender, EventArgs e)
        {
            plotCheckedListBoxEditTextbox.Visible = false;
            Settings.Default.plotSignals[plotCheckedListBox.SelectedIndex] = plotCheckedListBoxEditTextbox.Text;
            plotCheckedListBox.Items[plotCheckedListBox.SelectedIndex] = "[" + plotCheckedListBox.SelectedIndex.ToString() + "] " + Settings.Default.plotSignals[plotCheckedListBox.SelectedIndex];
            plotCheckedListBox.Focus();
        }


        private void plotCheckedListBoxEditTextbox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyValue == (int)Keys.Enter)
            {
                e.SuppressKeyPress = true;
                plotCheckedListBoxEditTextbox_Leave(sender, e);
            }
            else if (e.KeyValue == (int)Keys.Escape)
            {
                plotCheckedListBoxEditTextbox.Text = plotCheckedListBoxEditTextboxSavedText;
                e.SuppressKeyPress = true;
                plotCheckedListBoxEditTextbox_Leave(sender, e);
            }
        }

        private void baudToolStripMenuItem_Click(object sender, EventArgs e)
        {
            baudToolStripDropDownButton.Text = ((ToolStripMenuItem)sender).Text;
        }

        private void connectToolStripSplitButtonMenuItem_Click(object sender, EventArgs e)
        {
            connectToolStripSplitButton.Text =  ((ToolStripMenuItem)sender).Text;
        }

        private void connectToolStripSplitButton_DropDownOpening(object sender, EventArgs e)
        {
            if (!serialPort.IsOpen)
            {
                connectToolStripSplitButton.DropDownItems.Clear();                

                List<string> ports = new List<string>();
                using (var searcher = new ManagementObjectSearcher("SELECT * FROM WIN32_SerialPort"))
                {
                    string[] portnames = SerialPort.GetPortNames();
                    var xports = searcher.Get().Cast<ManagementBaseObject>().ToList();
                    var tList = (from n in portnames join p in xports on n equals p["DeviceID"].ToString() select n + " - " + p["Caption"]).ToList();

                    for (int i = 0; i < tList.Count; i++)
                    {
                        ports.Add(tList[i].Split(' ')[0]);
                        /*if (tList[i].Contains("Special COM Port name"))
                        {
                            
                        }*/
                    }
                }

                if (ports.Count > 0)
                {
                    foreach (string s in ports)
                    {
                        ToolStripMenuItem mi = new ToolStripMenuItem(s) { Name = s + "ToolStripMenuItem" };
                        mi.Click += connectToolStripSplitButtonMenuItem_Click;
                        connectToolStripSplitButton.DropDownItems.Add(mi);

                        if (connectToolStripSplitButton.Text == "no ports")
                        {
                            connectToolStripSplitButton.Text = s;
                        }
                    }

                    logToolStripStatusLabel.Text = ports.Count.ToString() + " port(s) found";
                }
                else
                {
                    logToolStripStatusLabel.Text = "no ports found";
                    connectToolStripSplitButton.Text = "no ports";
                    connectToolStripSplitButton.ForeColor = Color.Red;
                }
            }
            else
            {
                titanOnDisconnect();
            }
        }

        private void connectToolStripSplitButton_ButtonClick(object sender, EventArgs e)
        {
            if (!serialPort.IsOpen)
            {
                titanOnConnect();
            }
            else
            {
                titanOnDisconnect();
            }
        }

        private void connectionTimer_Tick(object sender, EventArgs e)
        {
            if(!serialPort.IsOpen)
            {
                titanOnDisconnect();
                logToolStripStatusLabel.Text = "lost connection to " + serialPort.PortName;
            }
            else
            {
                int speed = (titan_receiveBytes - titan_lastReceiveBytes);
                titan_lastReceiveBytes = titan_receiveBytes;

                if(speed * 10 <= bandwidthToolStripProgressBar.Maximum)
                {
                    bandwidthToolStripProgressBar.Value = speed * 10;
                }

                string unit = "B/s";
                if (speed > 10000)
                {
                    speed /= 1024;
                    unit = "KB/s";
                }

                if (speed > 10000)
                {
                    speed /= 1024;
                    unit = "MB/s";
                }
                speedToolStripStatusLabel.Text = speed.ToString() + " " + unit;
                bandwidthToolStripProgressBar.ToolTipText = speed.ToString() + " " + unit;
            }
        }

        private void titanOnConnect()
        {
            if ((!serialPort.IsOpen) && (connectToolStripSplitButton.Text != "no ports"))
            {
                try
                {
                    serialPort.PortName = connectToolStripSplitButton.Text;
                    serialPort.BaudRate = Convert.ToInt32(baudToolStripDropDownButton.Text);

                    bandwidthToolStripProgressBar.Maximum = serialPort.BaudRate;

                    serialPort.Open();
                    string dump = serialPort.ReadExisting();
                    connectionTimer.Enabled = true;

                    baudToolStripDropDownButton.Enabled = false;

                    connectToolStripSplitButton.ForeColor = Color.Green;

                    logToolStripStatusLabel.Text = "connnected";
                }
                catch (Exception ex)
                {
                    titanOnDisconnect();
                    logToolStripStatusLabel.Text = "error on connect";
                }
            }
        }

        private void titanOnDisconnect()
        {
            try
            {
                connectionTimer.Enabled = false;
                serialPort.Close();

                baudToolStripDropDownButton.Enabled = true;
                connectToolStripSplitButton.ForeColor = Color.Red;
                speedToolStripStatusLabel.Text = "";
                bandwidthToolStripProgressBar.Value = 0;
            }
            catch (Exception ex)
            {
                logToolStripStatusLabel.Text = "error on disconnect";
            }
        }

        private void serialPort_DataSend(byte command, byte[] data)
        {
            int dl = 0;
            if (data != null)
            {
                dl = data.Length;
            }
            byte[] buffer = new byte[8 + dl];
            buffer[0] = Convert.ToByte('t');
            buffer[1] = Convert.ToByte('2');
            buffer[2] = Convert.ToByte('c');
            buffer[3] = Convert.ToByte(command);
            buffer[4] = Convert.ToByte(dl % 256);
            buffer[5] = Convert.ToByte(dl / 256);
            if (data != null)
            {
                data.CopyTo(buffer, 6);
            }
            UInt16 checksum = new UInt16();
            for (int i = 0; i < buffer.Length - 2; i++)
            {
                checksum += buffer[i];
            }
            buffer[buffer.Length - 2] = Convert.ToByte(checksum % 256);
            buffer[buffer.Length - 1] = Convert.ToByte(checksum / 256);

            try
            {
                serialPort.Write(buffer, 0, buffer.Length);
            }
            catch (Exception ex)
            {
                titanOnDisconnect();
                logToolStripStatusLabel.Text = "error on DataSend";
            }
        }

        private void serialPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            byte[] readbuffer = new byte[serialPort.ReadBufferSize];
            int bytesRead = 0;
            try
            {
                bytesRead = serialPort.Read(readbuffer, 0, readbuffer.Length);
                Array.Resize(ref readbuffer, bytesRead);
            }
            catch (Exception ex)
            {
                titanOnDisconnect();
                logToolStripStatusLabel.Text = "error on DataReceived";
            }

            titan_receiveBytes += bytesRead;
            titanProcessData(readbuffer);
        }

        private void titanProcessData(byte[] buffer)
        {
            int i = 0;
            while (i < buffer.Length)
            {
                titan_receiveCommand[titan_receiveCommandLength++] = buffer[i++];

                switch (titan_receiveState)
                {
                    case 0: //waiting for start "t2c"
                        switch (titan_receiveCommandLength)
                        {
                            case 1:
                                if (titan_receiveCommand[0] != 't')
                                {
                                    titan_receiveCommandLength = 0;
                                }
                                break;

                            case 2:
                                if (titan_receiveCommand[1] != '2')
                                {
                                    if (titan_receiveCommand[1] == 't')
                                    {
                                        titan_receiveCommand[0] = Convert.ToByte('t');
                                        titan_receiveCommandLength = 1;
                                    }
                                    else
                                    {
                                        titan_receiveCommandLength = 0;
                                    }
                                }
                                break;

                            case 3:
                                if (titan_receiveCommand[2] != 'c')
                                {
                                    if (titan_receiveCommand[2] == 't')
                                    {
                                        titan_receiveCommand[0] = Convert.ToByte('t');
                                        titan_receiveCommandLength = 1;
                                    }
                                    else
                                    {
                                        titan_receiveCommandLength = 0;
                                    }
                                }
                                else
                                {
                                    titan_receiveState = 1;
                                }
                                break;
                        }
                        break;

                    case 1: //waiting for metadata
                        if (titan_receiveCommandLength == 6)
                        {
                            titan_receiveWaitLength = titan_receiveCommand[4] + 256 * titan_receiveCommand[5];
                            if (titan_receiveWaitLength == 0)
                            {
                                titan_receiveWaitLength = 2;
                                titan_receiveState = 3;
                            }
                            else
                            {
                                titan_receiveState = 2;
                            }
                        }
                        break;

                    case 2:     //waiting for data
                        titan_receiveWaitLength--;
                        if (titan_receiveWaitLength == 0)
                        {
                            titan_receiveWaitLength = 2;
                            titan_receiveState = 3;
                        }
                        break;

                    case 3:     //waiting for checksum
                        titan_receiveWaitLength--;
                        if (titan_receiveWaitLength == 0)
                        {
                            UInt16 checksum_read = new UInt16();
                            UInt16 checksum_calc = new UInt16();
                            checksum_read = Convert.ToUInt16(titan_receiveCommand[titan_receiveCommandLength - 2] + 256 * titan_receiveCommand[titan_receiveCommandLength - 1]);
                            checksum_calc = 0;
                            for (int j = 0; j < titan_receiveCommandLength - 2; j++)
                            {
                                checksum_calc += titan_receiveCommand[j];
                            }

                            if (checksum_read == checksum_calc)
                            {
                                byte[] command = new byte[titan_receiveCommandLength - 2];
                                Array.Copy(titan_receiveCommand, command, titan_receiveCommandLength - 2);

                                if (InvokeRequired)
                                {
                                    this.BeginInvoke(new Action(() => guiProcessCommand(command)));
                                }
                            }
                            else
                            {
                                if (InvokeRequired)
                                {
                                    this.BeginInvoke(new Action(() => { logToolStripStatusLabel.Text = "comm CRC error"; }));
                                }
                            }

                            titan_receiveState = 0;
                            titan_receiveCommandLength = 0;
                        }
                        break;
                }
            }
        }

        private void guiProcessCommand(byte[] command_array)
        {
            byte command_no = command_array[3];
            int data_length = command_array[4] + 256 * command_array[5];
            byte[] data = new byte[data_length];
            Array.Copy(command_array, 6, data, 0, data_length);
            command_array = null;

            System.UInt32 channel;
            System.UInt32 y;
            System.Int32 z;

            switch (command_no)
            {
                case 0x83:   //log
                    logWindow.logAdd(data);
                    break;

                case 0x80:   //plot single (any of 1, 2 or 4 BYTE)
                    channel = Convert.ToUInt32(data[0]);
                    z = 0;
                    if (data_length == 2)
                    {
                        z = unchecked((sbyte)data[1]);
                    }
                    else if(data_length == 3)
                    {
                        uint d1 = data[1];
                        uint d2 = data[2];
                        uint temp = d1 | (d2 << 8);
                        z = unchecked((Int16)temp);
                    }
                    else
                    {
                        uint d1 = data[1];
                        uint d2 = data[2];
                        uint d3 = data[3];
                        uint d4 = data[4];
                        uint temp = d1 | (d2 << 8) | (d3 << 16) | (d4 << 24);
                        z = unchecked((Int32)temp);
                    }
                    
                    foreach (graphForm graph in graphWindow)
                    {
                        graph.addData(channel, z);
                    }
                    break;

                case 0x81:   //plot multiple 1B values
                    channel = Convert.ToUInt32(data[0]);
                    for (int i = 1; i < data.Length; i++)
                    {
                        z = unchecked((sbyte)data[i]);

                        foreach (graphForm graph in graphWindow)
                        {
                            graph.addData(channel, z);
                        }
                    }
                    break;

                case 0x82:   //plot multiple 2B values
                    channel = Convert.ToUInt32(data[0]);
                    for (int i = 1; i < data.Length; i += 2)
                    {
                        uint d1 = data[i];
                        uint d2 = data[i + 1];
                        uint temp = d1 | (d2 << 8);
                        z = unchecked((Int16)temp);

                        foreach (graphForm graph in graphWindow)
                        {
                            graph.addData(channel, z);
                        }
                    }
                    break;

                case 0x84:   //plot multiple 4B values
                    channel = Convert.ToUInt32(data[0]);
                    for (int i = 1; i < data.Length; i += 4)
                    {
                        uint d1 = data[i];
                        uint d2 = data[i + 1];
                        uint d3 = data[i + 2];
                        uint d4 = data[i + 3];
                        uint temp = d1 | (d2 << 8) | (d3 << 16) | (d4 << 24);
                        z = unchecked((Int32)temp);

                        foreach (graphForm graph in graphWindow)
                        {
                            graph.addData(channel, z);
                        }
                    }
                    break;

                case 0x86:   //plot single (any of 1, 2 or 4 BYTE)
                    channel = Convert.ToUInt32(data[0]);
                    y = 0;
                    if (data_length == 2)
                    {
                        y = Convert.ToUInt32(data[1]);
                    }
                    else if (data_length == 3)
                    {
                        uint d1 = data[1];
                        uint d2 = data[2];
                        y = d1 | (d2 << 8);
                    }
                    else
                    {
                        uint d1 = data[1];
                        uint d2 = data[2];
                        uint d3 = data[3];
                        uint d4 = data[4];
                        y = d1 | (d2 << 8) | (d3 << 16) | (d4 << 24);
                    }

                    foreach (graphForm graph in graphWindow)
                    {
                        graph.addData(channel, y);
                    }
                    break;

                case 0x87:   //plot multiple 1B values
                    channel = Convert.ToUInt32(data[0]);
                    for (int i = 1; i < data.Length; i++)
                    {
                        y = Convert.ToUInt32(data[i]);
                        foreach (graphForm graph in graphWindow)
                        {
                            graph.addData(channel, y);
                        }
                    }
                    break;

                case 0x88:   //plot multiple 2B values
                    channel = Convert.ToUInt32(data[0]);
                    for (int i = 1; i < data.Length; i += 2)
                    {
                        uint d1 = data[i];
                        uint d2 = data[i + 1];
                        y = d1 | (d2 << 8);
                        foreach (graphForm graph in graphWindow)
                        {
                            graph.addData(channel, y);
                        }
                    }
                    break;

                case 0x89:   //plot multiple 4B values
                    channel = Convert.ToUInt32(data[0]);
                    for (int i = 1; i < data.Length; i += 4)
                    {
                        uint d1 = data[i];
                        uint d2 = data[i + 1];
                        uint d3 = data[i + 2];
                        uint d4 = data[i + 3];
                        y = d1 | (d2 << 8) | (d3 << 16) | (d4 << 24);
                        foreach (graphForm graph in graphWindow)
                        {
                            graph.addData(channel, y);
                        }
                    }
                    break;
            }
        }

        private void alwaysOnTopToolStripMenuItem_CheckedChanged(object sender, EventArgs e)
        {
            TopMost = alwaysOnTopToolStripMenuItem.Checked;
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Close();
            Application.Exit();
        }

        private void logToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (logWindow.Visible)
            {
                logWindow.Hide();
            }
            else
            {
                logWindow.Show();
            }
        }

        private void aboutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            aboutBox a = new aboutBox();
            a.Show();
        }

        private void bindFromLog_KeyDown(object sender, KeyEventArgs e)
        {
            if(bindLogCheckBox.Checked)
            {
                bindTextBox_KeyDown(sender, e);
            }
        }

        private void bindFromGraph_KeyDown(object sender, KeyEventArgs e)
        {
            if (bindGraphCheckBox.Checked)
            {
                bindTextBox_KeyDown(sender, e);
            }
        }

        private void bindTextBox_KeyDown(object sender, KeyEventArgs e)
        {
            int keyValue = e.KeyValue;
            int keyCode = 0;
            if (keyValue >= (int)Keys.A && keyValue <= (int)Keys.Z)
            {
                keyCode = keyValue; 
            }

            if (keyValue >= (int)Keys.D0 && keyValue <= (int)Keys.D9)
            {
                keyCode = keyValue;
            }

            if (keyCode == 0)
            {
                bindLogTextBox.Text = "Invalid key pressed.";
            }
            else
            {
                bindLogTextBox.Text = "Key " + Convert.ToChar(keyCode) + " (code: " + keyCode.ToString() + ") pressed.";
                byte command = 0x85;
                byte[] data = new byte[1];
                data[0] = Convert.ToByte(keyCode);

                serialPort_DataSend(command, data);
            }

            e.SuppressKeyPress = true;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            byte command = 0x00;
            byte[] data = new byte[1024];
            for(int i = 0; i < 1024; i++)
            {
                data[i] = Convert.ToByte(i % 51);
            }

            serialPort_DataSend(command, data);
            logToolStripStatusLabel.Text = "sent garbage 0";
        }

        private void button2_Click(object sender, EventArgs e)
        {
            byte command = 0x01;
            byte[] data = new byte[28];
            for (int i = 0; i < 28; i++)
            {
                data[i] = Convert.ToByte(18 + i);
            }

            serialPort_DataSend(command, data);
            logToolStripStatusLabel.Text = "sent garbage 1";
        }
    }
}
