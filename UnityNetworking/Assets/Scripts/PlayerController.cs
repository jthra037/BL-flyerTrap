using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Networking;

[RequireComponent(typeof(Rigidbody))]
public class PlayerController : NetworkBehaviour
{
    [SerializeField] private GameObject bulletPrefab;
    [SerializeField] private Transform bulletSpawn;
    [SerializeField] private GameObject body;
    [SerializeField] private Camera eyes;
    public RectTransform scoreHeading;
    private List<Text> scoreListings = new List<Text>();

    private GameManager gm;

    private Rigidbody rb;
    private AudioSource myAs;

    [SerializeField]
    private float moveSpeed = 10;
    [SerializeField]
    private float maxSpeed = 5;
    [SerializeField]
    private float rotSpeed = 10;
    [SerializeField]
    private float airbourneModifier = 0.2f;

    private bool grounded = true;
    private bool jumping = false;
    private bool falling = false;
    private bool isSprinting = false;

    private float jumpSpeed;

    // Use this for initialization
    void Start()
    {
        rb = GetComponent<Rigidbody>();
        myAs = GetComponent<AudioSource>();

        rb.maxAngularVelocity = (1.5f * Mathf.PI); // Make sure we don't spin too fast

        //jumpSpeed = FindReqJumpSpeed(2.6f); // Figure out what Viy should be to jump 2.6 units all the time
        jumpSpeed = FIN.FindViForPeak(2.6f);

        gm = GameObject.FindGameObjectWithTag("GameController").GetComponent<GameManager>();
        Debug.Log(SwitchBoard.gm);
        Debug.Log(SwitchBoard.gm == gm ? "Nothing seems wrong, switchboard has gm" : "Switchboard.gm != gm");

        // register to delegate
        GameManager.ScoreAction += HUDScoreUpdate;
    }

    public override void OnStartLocalPlayer()
    {
        body.GetComponent<MeshRenderer>().material.color = Color.blue;
        eyes.gameObject.SetActive(true);
    }

    // Mostly just move states around in here
    private void Update()
    {
        if (!isLocalPlayer)
        {
            return;
        }
        // If player is spinning but not trying to turn
        if (Input.GetAxis("Horizontal") == 0 &&
            rb.angularVelocity.sqrMagnitude > Mathf.Epsilon)
        {
            rb.angularDrag = 12; // stop them from spinning
        }
        else
        {
            rb.angularDrag = 0; // let them rotate freely
        }

        // keep track of when player is not holding jump
        if (jumping && Input.GetAxisRaw("Jump") == 0)
        {
            jumping = false;
        }

        // keep track of when the player is falling
        falling = rb.velocity.y < 0 && Mathf.Abs(rb.velocity.y) > Mathf.Epsilon;

        if (Input.GetMouseButtonDown(0))
        {
            CmdFire();
        }
    }

    // Update is called once per frame
    private void FixedUpdate()
    {
        if (!isLocalPlayer)
        {
            return;
        }

        // figure out if we should be hampering the players controls because they are in the air
        float airDamp = grounded ? 1 : airbourneModifier;
        float sprintMod = Input.GetKey(KeyCode.LeftShift) ? 2 : 1;

        // update the velocity based on inputs relative to transform
        Vector3 vel = rb.velocity;
        Vector3 forwVel = transform.forward * moveSpeed * airDamp * Input.GetAxisRaw("Vertical") * sprintMod;
        Vector3 rightVel = transform.right * moveSpeed * airDamp * Input.GetAxisRaw("Horizontal") * sprintMod;
        vel.z = forwVel.z + rightVel.z;
        vel.x = forwVel.x + rightVel.x;
        rb.velocity = vel;

        if (grounded &&
            Input.GetAxis("Jump") != 0 &&
            !jumping)
        {
            jumping = true; // player is now holding the jump button
            rb.AddForce(transform.up * jumpSpeed, ForceMode.VelocityChange); //add jump Viy we found in Start()
        }
        else if (Input.GetAxis("Jump") != 0)
        {
            grounded = false;
        }

        // looks less floaty if gravity is more intense during falls
        // this makes projectiles way harder to calculate though, so turn it off sometimes
        if (!grounded && falling)
        {
            rb.AddForce(2 * Physics.gravity);
        }
    }

    private void OnTriggerEnter(Collider other)
    {
        if (other.CompareTag("Flag"))
        {
            pickupObject(other.transform);
            gm.FlagHolderUpdate(this);
        }
    }

    private void OnTriggerStay(Collider other)
    {
        // if our groundcheck trigger is on the ground, record that
        if (other.CompareTag("Ground"))
        {
            grounded = true;
        }
    }

    private void OnTriggerExit(Collider other)
    {
        // if our groundcheck trigger left the ground, record that
        if (other.CompareTag("Ground"))
        {
            grounded = false;
        }
    }

    // re-arrange Vf^2 = Vi^2 + 2ad to find Viy given height
    private float FindReqJumpSpeed(float height)
    {
        return Mathf.Sqrt(-2 * Physics.gravity.y * height);
    }

    [Command]
    private void CmdFire()
    {
        // Create the Bullet from the Bullet Prefab
        GameObject bullet = Instantiate(
            bulletPrefab,
            bulletSpawn.position,
            bulletSpawn.rotation);

        // Add velocity to the bullet
        bullet.GetComponent<Rigidbody>().velocity = bullet.transform.forward * 6;

        NetworkServer.Spawn(bullet);

        // Destroy the bullet after 2 seconds
        Destroy(bullet, 2.0f);
    }

    private void pickupObject(Transform o)
    {
        o.parent = bulletSpawn;

        o.localPosition = Vector3.zero;

        Quaternion desiredRotation = new Quaternion();
        desiredRotation.eulerAngles = new Vector3(0, 180, 60);
        o.localRotation = desiredRotation;
    }

    private void register()
    {
        gm.Register(this);
    }

    private void deregister()
    {
        gm.Deregister(this);
    }

    private void HUDScoreUpdate()
    {
        Debug.Log("HUDScoreUpdate called on " + GetInstanceID());
    }
}
