﻿using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Networking;

[RequireComponent(typeof(Rigidbody))]
public class PlayerController : NetworkBehaviour
{
    [SerializeField] private GameObject bulletPrefab;
    [SerializeField] private Transform bulletSpawn;

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
    }

    public override void OnStartLocalPlayer()
    {
        GetComponent<MeshRenderer>().material.color = Color.blue;
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

        if(Input.GetMouseButtonDown(0))
        {
            CmdFire();
        }
    }

    // Update is called once per frame
    private void FixedUpdate()
    {
        // figure out if we should be hampering the players controls because they are in the air
        float airDamp = grounded ? 1 : airbourneModifier;
        float sprintMod = Input.GetKey(KeyCode.LeftShift) ? 2 : 1;

        // apply the forces by multiplying relevant modifier and input axis
        rb.AddForce(transform.forward * moveSpeed * airDamp * Input.GetAxisRaw("Vertical") * sprintMod);
        //rb.AddForce(transform.right * moveSpeed * airDamp * Input.GetAxisRaw("Strafe") * sprintMod);
        rb.AddTorque(transform.up * rotSpeed * airDamp * Input.GetAxis("Horizontal"));

        Vector2 controllableMovement = new Vector2(rb.velocity.x, rb.velocity.z);
        if (controllableMovement.sqrMagnitude > (maxSpeed * maxSpeed * sprintMod))
        {
            controllableMovement = controllableMovement.normalized * maxSpeed * sprintMod;
            rb.velocity = new Vector3(controllableMovement.x, rb.velocity.y, controllableMovement.y);
        }

        // only allow player to jump if they are 
        //1) on the ground 
        //2) pressing the jump button 
        //3) if they are not still holding the jump button from a previous jump
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
        var bullet = (GameObject)Instantiate(
            bulletPrefab,
            bulletSpawn.position,
            bulletSpawn.rotation);

        // Add velocity to the bullet
        bullet.GetComponent<Rigidbody>().velocity = bullet.transform.forward * 6;

        NetworkServer.Spawn(bullet);

        // Destroy the bullet after 2 seconds
        Destroy(bullet, 2.0f);
    }


}