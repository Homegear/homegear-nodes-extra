<?php
declare(strict_types=1);

use PHPMailer\PHPMailer\PHPMailer;
use PHPMailer\PHPMailer\SMTP;
use PHPMailer\PHPMailer\Exception;

require_once 'PHPMailer/Exception.php';
require_once 'PHPMailer/PHPMailer.php';
require_once 'PHPMailer/SMTP.php';

class HomegearNode extends HomegearNodeBase
{
    private $hg = NULL;
    private $nodeInfo = NULL;

    public function __construct()
    {
	    $this->hg = new \Homegear\Homegear();
    }

    public function input(array $nodeInfoLocal, int $inputIndex, array $message)
    {
        $recipient = (isset($message['xmpp-recipient']) ? $message['xmpp-recipient'] : $nodeInfoLocal['info']['recipient']);
        $msg = (isset($message['xmpp-message']) ? $message['xmpp-message'] : $nodeInfoLocal['info']['message']);
        $config = $this->getConfigParameter($nodeInfoLocal['info']['server'],'port');
        $this->_sendMessage($config,$recipient,$msg);
    }

    private function _sendMessage($config = false, $recipient = false, $msg = false)
    {
        if ($config == false) { $this->log(4,"email-server: config parameters missing!"); return; }
        if ($recipient == false) { $this->log(4,"email-server: recipient missing!"); return; }
        if ($msg == false) { $this->log(4,"email-server: message missing!"); return; }

    }
}
?>