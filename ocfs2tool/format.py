import gtk

import ocfs2

from guiutil import set_props, error_box, format_bytes
from process import Process

class BaseCombo(gtk.Combo):
    def __init__(self):
        gtk.Combo.__init__(self)
        self.entry.set_editable(False)

class ValueCombo(BaseCombo):
    def __init__(self, minimum, maximum):
        BaseCombo.__init__(self)

        choices = ['Auto']

        size = minimum
        while size <= maximum:
            choices.append(format_bytes(size))
            size = size << 1

        self.set_popdown_strings(choices)

    def get_arg(self):
        text = self.entry.get_text()

        if text != 'Auto':
            s = text.replace(' ', '')
            s = s.replace('B', '')
            s = s.replace('bytes', '')
            return (self.arg, s)
        else:
            return None

class NumNodes(gtk.SpinButton):
    def __init__(self):
        adjustment = gtk.Adjustment(4, 2, ocfs2.MAX_NODES, 1, 10)
        gtk.SpinButton.__init__(self, adjustment=adjustment, )

    def get_arg(self):
        s = self.get_text()

        if s:
            return ('-n', s)
        else:
            return None

class Device(BaseCombo):
    def fill(self, partitions, device):
        for partition in partitions:
            item = gtk.ListItem(partition)
            item.show()
            self.list.add(item)

            if partition == device:
                item.select()

    def get_device(self):
        return self.entry.get_text()

class VolumeLabel(gtk.Entry):
    def __init__(self):
        gtk.Entry.__init__(self, max=ocfs2.MAX_VOL_LABEL_LEN)
        self.set_text('oracle')

    def get_arg(self):
        s = self.get_text()

        if s:
            return ('-L', s)
        else:
            return None

class ClusterSize(ValueCombo):
    def __init__(self):
        ValueCombo.__init__(self, ocfs2.MIN_CLUSTER_SIZE,
                                  ocfs2.MAX_CLUSTER_SIZE)
        self.arg = '-c'

class BlockSize(ValueCombo):
    def __init__(self):
        ValueCombo.__init__(self, ocfs2.MIN_BLOCKSIZE,
                                  ocfs2.MAX_BLOCKSIZE)
        self.arg = '-b'

entries = (
    ('Device', Device, False),
    ('Volume Label', VolumeLabel, False),
    ('Cluster Size', ClusterSize, False),
    ('Number of Nodes', NumNodes, False),
    ('Block Size', BlockSize, True)
)

def format_partition(parent, device, advanced):
    partitions = ocfs2.partition_list(True)

    if not partitions:
        error_box(parent, 'No unmounted partitions')
        return False

    if advanced:
        rows = 5
    else:
        rows = 4

    dialog = gtk.Dialog(parent=parent, title='Format',
                        buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                 gtk.STOCK_OK,     gtk.RESPONSE_OK))

    table = gtk.Table(rows=rows, columns=2)
    set_props(table, row_spacing=4,
                     column_spacing=4,
                     border_width=4,
                     parent=dialog.vbox)

    partitions.sort()

    widgets = []
    row = 0

    for desc, widget_type, advanced_only in entries:
        if advanced_only and not advanced:
            continue

        label = gtk.Label(desc + ':')
        set_props(label, xalign=1.0)
        table.attach(label, 0, 1, row, row + 1)

        widget = widget_type()
        table.attach(widget, 1, 2, row, row + 1)

        if widget_type == Device:
            widget.fill(partitions, device)

        widgets.append(widget)

        row = row + 1

    widgets[0].grab_focus()

    dialog.show_all()
 
    if dialog.run() != gtk.RESPONSE_OK:
        dialog.destroy()
        return False

    dev = widgets[0].get_device()
    msg = 'Are you sure you want to format %s?' % dev

    ask = gtk.MessageDialog(parent=dialog,
                            flags=gtk.DIALOG_DESTROY_WITH_PARENT,
                            type=gtk.MESSAGE_QUESTION,
                            buttons=gtk.BUTTONS_YES_NO,
                            message_format=msg)

    if ask.run() != gtk.RESPONSE_YES:
        dialog.destroy()
        return False

    command = ['mkfs.ocfs2']
    for widget in widgets[1:]:
        arg = widget.get_arg()

        if arg:
            command.extend(arg)

    command.append(dev)

    dialog.destroy()

    mkfs = Process(command, 'Format', 'Formatting...', parent)
    success, output, k = mkfs.reap()

    if not success:
        error_box(parent, 'Format error: %s' % output)
        return False

    return True

def main():
    format(None, None, True)

if __name__ == '__main__':
    main()