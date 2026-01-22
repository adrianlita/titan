using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using DarkUI.Forms;

namespace TitanExplorer
{
    public partial class logForm : DarkForm
    {
        public int savedHeight;

        public logForm()
        {
            InitializeComponent();
        }

        private void logForm_Resize(object sender, EventArgs e)
        {
            if (WindowState != FormWindowState.Minimized)
            {
                savedHeight = Height;
            }
        }

        public void logAdd(byte[] data)
        {
            string new_data = System.Text.Encoding.ASCII.GetString(data);
            if(logTimestampCheckBox.Checked)
            {
                string timestamp = new DateTimeOffset(DateTime.UtcNow).ToUnixTimeSeconds().ToString() + DateTime.UtcNow.ToString(".fff");
                new_data = "[" + timestamp + "] " + new_data;
            }

            if (logFreezeCheckBox.Checked)
            {
                logTextBoxHidden.AppendText(new_data);
            }
            else
            {
                if (logTextBoxHidden.Text != "")
                {
                    logTextBox.AppendText(logTextBoxHidden.Text);
                    logTextBoxHidden.Text = "";
                }
                logTextBox.AppendText(new_data);
            }

            logBufferProgressBar.Value = (logTextBox.Text.Length + logTextBoxHidden.Text.Length) * 99 / logTextBox.MaxLength + 1;
        }

        private void logClearButton_Click(object sender, EventArgs e)
        {
            logTextBoxHidden.Clear();
            logTextBox.Clear();
            if (logFreezeCheckBox.Checked)
            {
                logFreezeCheckBox.Checked = false;
                logFreezeCheckBox.Update();
            }
        }

        private void logSaveButton_Click(object sender, EventArgs e)
        {
            saveFileDialog.Filter = "LOG files|*.log";
            if (saveFileDialog.ShowDialog() == DialogResult.OK)
            {
                File.WriteAllText(saveFileDialog.FileName, logTextBox.Text);
            }
        }

        private void alwaysOnTopCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            TopMost = alwaysOnTopCheckBox.Checked;
        }

        
    }
}
